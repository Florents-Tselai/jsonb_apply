#include "postgres.h"

#include "fmgr.h"
#include "funcapi.h"
#include "utils/builtins.h"
#include "utils/jsonb.h"
#include "utils/jsonfuncs.h" /* transform_jsonb_string_values */
#include "utils/formatting.h"
#include "catalog/pg_proc.h"
#include "utils/regproc.h"
#include "utils/syscache.h"
#include "access/htup_details.h"
#include "parser/parse_func.h"
#if PG_VERSION_NUM >= 160000

#include "varatt.h"

#endif

#if PG_VERSION_NUM < 130000
#define TYPALIGN_DOUBLE 'd'
#define TYPALIGN_INT 'i'
#endif


PG_MODULE_MAGIC;

void
_PG_init(void) {
}

void _PG_fini(void) {
}


/* A struct to pass around as a "callable" */
typedef struct Func {
    // text *indef; /* As provided by the user */

    Datum proc; /* proc OID */
    FmgrInfo *finfo;
    Form_pg_proc form; /* corresponds to a pointer to a tuple with the format of pg_proc relation. */
} Func;


typedef struct JsonbApplyState {
    FunctionCallInfo top_fcinfo; /* fcinfo from top jsonb_apply(PG_FUNCTION_ARGS)*/
    Datum funcregproc; /* the fucnregproc to be applied */
    // int nargs;
    Form_pg_proc procStruct; /*  */
    const Datum *funcargs1_n; /* arg0 is the json value itself, additional are supplied by the user. */
    const Oid *funcargs1_n_types;
    Oid fncollation;
    Func *f;
} JsonbApplyState;

#define FUNC_OID(f) (DatumGetObjectId((f)->proc))


static text *
variadic_apply_func_jsonb_value(void *_state, char *elem_value, int elem_len) {
    JsonbApplyState *state = (JsonbApplyState *) _state;
    Func *f = state->f;
    Oid collation = state->fncollation;

    int f_nargs = f->form->pronargs;
    PGFunction fn = f->finfo->fn_addr;
    const Datum *args1_n = state->funcargs1_n;

    Datum result;

    /* Can't figure out why some funcs need a copy. So making them all use copy. */
    char *elemcopy = (char *) malloc(elem_len + 1);
    if (elemcopy != NULL) {
        memcpy(elemcopy, elem_value, elem_len);
        elemcopy[elem_len] = '\0';
    }

    Datum arg0 = CStringGetTextDatum(elemcopy);

    switch (f_nargs) {
        case 0:
            result = FunctionCall0Coll(f->finfo, collation);
            break;
        case 1:
            result = DirectFunctionCall1Coll(fn, collation, arg0);
            break;
        case 2:
            result = DirectFunctionCall2Coll(fn, collation, arg0, args1_n[0]);
            break;
        case 3:
            result = DirectFunctionCall3Coll(fn, collation, arg0, args1_n[0], args1_n[1]);
            break;
        case 4:
            result = DirectFunctionCall4Coll(fn, collation, arg0, args1_n[0], args1_n[1], args1_n[2]);
            break;
        case 5:
            result = DirectFunctionCall5Coll(fn, collation, arg0, args1_n[0], args1_n[1], args1_n[2],
                                             args1_n[3]);
            break;
        case 6:
            result = DirectFunctionCall6Coll(fn, collation, arg0, args1_n[0], args1_n[1], args1_n[2],
                                             args1_n[3], args1_n[4]);
            break;
        case 7:
            result = DirectFunctionCall7Coll(fn, collation, arg0, args1_n[0], args1_n[1], args1_n[2],
                                             args1_n[3], args1_n[4], args1_n[5]);
            break;
        case 8:
            result = DirectFunctionCall8Coll(fn, collation, arg0, args1_n[0], args1_n[1], args1_n[2],
                                             args1_n[3], args1_n[4], args1_n[5], args1_n[6]);
            break;
        case 9:
            result = DirectFunctionCall9Coll(fn, collation, arg0, args1_n[0], args1_n[1], args1_n[2],
                                             args1_n[3], args1_n[4], args1_n[5], args1_n[6], args1_n[7]);
            break;
        default:
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("functions with more than 9 arguments are not supported")));
    }

    return DatumGetTextPP(result);;
}


static Datum
jsonb_apply_worker(Jsonb *jb, text *funcdef,
                   int nargs,
                   bool withvarargs, int nvarargs,
                   const Datum *varargs, const bool *varnulls,
                   const Oid *varargstypes, Oid collation) {
    HeapTuple tuple;

    Func *f = palloc0(sizeof(Func));
    // f->indef = funcdef;

    JsonbApplyState *state = palloc0(sizeof(JsonbApplyState));

    state->f = f;
    // state->nargs = nargs;
    state->fncollation = collation;

    /* without varargs */
    if (!withvarargs) {
        state->funcargs1_n = NULL;
        state->funcargs1_n_types = NULL;
        /* withvarargs */
    } else {
        state->funcargs1_n = varargs;
        state->funcargs1_n_types = varargstypes;
    }

    /* Search for the function in the catalog and fill-in the Func struct */

    bool funcdef_withargtypes = false;

    /* e.g. func(text,text,integer)  */
    if (funcdef_withargtypes) {
        if (!DirectInputFunctionCallSafe(regprocedurein, text_to_cstring(funcdef),
                                         InvalidOid, -1,
                                         NULL,
                                         &f->proc))

            ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("Invalid function name: %s", text_to_cstring(funcdef))));
    } else {
        /* we only have a function name and have to infer the argument types */

        int lookup_nargs = (withvarargs ? 1 + nvarargs : 1);
        Oid *lookup_argtypes = (Oid *) palloc0(lookup_nargs * sizeof(Oid));
        lookup_argtypes[0] = TEXTOID;

        for (int i = 1; i < lookup_nargs; i++)
            lookup_argtypes[i] = varargstypes[i - 1];

        Oid procOid = LookupFuncName(list_make1(makeString(text_to_cstring(funcdef))),
                                     lookup_nargs,
                                     lookup_argtypes,
                                     false);
        pfree(lookup_argtypes);

        if (!OidIsValid(procOid))
            ereport(ERROR, (errmsg("Function %s not found", text_to_cstring(funcdef))));

        f->proc = ObjectIdGetDatum(procOid);
    }


    /* Lets' fill in the state->procStruct which contains info about the function we call (see pg_proc relation) */
    tuple = SearchSysCache1(PROCOID, f->proc);
    if (!HeapTupleIsValid(tuple))
        elog(ERROR, "cache lookup failed for function %u", FUNC_OID(f));
    f->form = (Form_pg_proc) GETSTRUCT(tuple);
    ReleaseSysCache(tuple);

    f->finfo = palloc0(sizeof(FmgrInfo));
    fmgr_info(FUNC_OID(f), f->finfo);


    /* debug/inspect the function we found */
    {
        printf("variadic: proname=%s\tpronargs=%d\tprorettype=%d\n",
               NameStr(f->form->proname),
               f->form->pronargs,
               f->form->prorettype
        );
    }

    /* Sanity checks  */
    {
        if (f->form->prorettype != TEXTOID)
            elog(ERROR, "function %s does not return text", NameStr(f->form->proname));
    }

    JsonTransformStringValuesAction action = (JsonTransformStringValuesAction) variadic_apply_func_jsonb_value;

    Jsonb *out = transform_jsonb_string_values(jb, state, action);

    pfree(state);
    pfree(f->finfo);
    pfree(f);

    PG_RETURN_JSONB_P(out);
}

/* Implementation for jsonb_apply(jsonb, text/regproc/regprocedure, variadic "any" default null) */
Datum jsonb_apply_internal(FunctionCallInfo fcinfo, bool withfilter) {
    /* Input */
    Jsonb   *jb;                    /* That's always the first argument */
    Datum   fnKey;                  /* The function the user says wants to apply, we toggle on its Oid later*/

    bool variadic_null = false;     /* Are we in a variadic = null case ? */
    int variadic_start;             /* At what index variadic args start ? */

    bool withvarargs;
    int nvarargs;
    Datum *varargs;
    bool *varnulls;
    Oid *vartypes;

    /* Function metadata */
    Oid fnOid;                      /* The Oid we'll be looking for: oid of the function the user actually wants */
    FmgrInfo fnFinfo;
    Form_pg_proc fnForm;             /* how fn appears in the pg_proc relation*/
    PGFunction fn;

    if (!withfilter) {
        /* jsonb_apply(jsonb, text/regproc/regprocedure, args) */
        jb = PG_GETARG_JSONB_P(0);
        fnKey = PG_GETARG_DATUM(1);
        Assert((funcdefDatum == TEXTOID) || (funcdefOid == REGPROCOID));
        fnKey = PG_GETARG_DATUM(1);
        variadic_start = 2;
        variadic_null = PG_NARGS() == 3 && PG_ARGISNULL(2);
    } else {
        /*
         * jsonb_apply(jsonb, text[], text/regproc/regprocedure, args).
         * We have a filter which is idx_arg=1,
         * so we apply the filter first,
         * and then like the if-block above but the rest of the indices are shifted by +1
         */
        Datum filter = PG_GETARG_DATUM(1);
        /* We apply the filter before anything else */
        jb = DatumGetJsonbP(DirectFunctionCall2(jsonb_extract_path, PG_GETARG_DATUM(0), filter));
        fnKey = PG_GETARG_DATUM(2);

        variadic_start = 3;
        variadic_null = PG_NARGS() == 4 && PG_ARGISNULL(3);
    }

    /* extract variadic args and their metadata */
    nvarargs = extract_variadic_args(fcinfo, variadic_start, true, &varargs, &vartypes, &varnulls);

    withvarargs = (nvarargs == -1 || variadic_null) ? false : true;

    /* Now that we have info about the arguments we can combine them with fnKey to search for the function */

    /* We start by creating the array of argument types to look up with.
     * The first argument is assumed to be of text type (the individual Json string).
     * The rest are fetched from the other varargs.
     */

    int lookup_nargs = (withvarargs ? 1 + nvarargs : 1);
    Oid *lookup_argtypes = (Oid *) palloc0(lookup_nargs * sizeof(Oid));
    lookup_argtypes[0] = TEXTOID;

    for (int i = 1; i < lookup_nargs; i++)
        lookup_argtypes[i] = vartypes[i - 1];

    /* Lookup the function */
    switch (DatumGetObjectId(fnKey)) {
        case TEXTOID: {
            fnOid = LookupFuncName(list_make1(makeString(text_to_cstring(DatumGetTextP(fnKey)))),
                                     lookup_nargs,
                                     lookup_argtypes,
                                     false);
        }
        default:
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("to specify a function you can only use its name")));

    }

    HeapTuple tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(fnOid));
    if (!HeapTupleIsValid(tuple))
        elog(ERROR, "cache lookup failed for function %u", fnOid);
    fnForm = (Form_pg_proc) GETSTRUCT(tuple);
    ReleaseSysCache(tuple);

    // f->finfo = palloc0(sizeof(FmgrInfo));
    // fmgr_info(FUNC_OID(f), f->finfo);
    JsonbApplyState *state = palloc0(sizeof(JsonbApplyState));


    PG_RETURN_DATUM(
        jsonb_apply_worker(jb, funcdef, PG_NARGS(), withvarargs, nvarargs, varargs, varnulls, vartypes, fcinfo->
            fncollation));
}

PG_FUNCTION_INFO_V1(jsonb_apply_withfilter);

Datum
jsonb_apply_withfilter(PG_FUNCTION_ARGS) {
    return jsonb_apply_internal(fcinfo, true);
}

PG_FUNCTION_INFO_V1(jsonb_apply_nofilter);

Datum
jsonb_apply_nofilter(PG_FUNCTION_ARGS) {
    return jsonb_apply_internal(fcinfo, false);
}
