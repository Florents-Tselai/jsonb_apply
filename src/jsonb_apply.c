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
    text *indef; /* As provided by the user */

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
    Datum arg0;
    CStringGetTextDatum(elem_value);
    Datum *args1_n = state->funcargs1_n;

    Datum result;

    /* Can't figure out why some funcs need a copy. So making them all use copy. */
    char *elemcopy = (char *) malloc(elem_len + 1);
    if (elemcopy != NULL) {
        memcpy(elemcopy, elem_value, elem_len);
        elemcopy[elem_len] = '\0';
    }

    arg0 = CStringGetTextDatum(elemcopy);

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
                   const Oid *vartypes, Oid collation) {
    HeapTuple tuple;

    Func *f = palloc0(sizeof(Func));
    f->indef = funcdef;

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
        state->funcargs1_n_types = vartypes;
    }

    /* Search for the function in the catalog and fill-in the Func struct */

    if (!DirectInputFunctionCallSafe(regprocedurein, text_to_cstring(funcdef),
                                     InvalidOid, -1,
                                     NULL,
                                     &f->proc))

        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("Invalid function name: %s", text_to_cstring(funcdef))));


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

PG_FUNCTION_INFO_V1(jsonb_apply);

Datum
jsonb_apply(PG_FUNCTION_ARGS) {
    Jsonb *jb = PG_GETARG_JSONB_P(0);
    text *funcdef = PG_GETARG_TEXT_PP(1);

    Datum *varargs;
    bool *varnulls;
    Oid *vartypes;

    /* build argument values to build the object */
    int nvarargs = extract_variadic_args(fcinfo, 2, true,
                                         &varargs, &vartypes, &varnulls);

    bool withvarargs = (nvarargs == -1) ? false : true;

    PG_RETURN_DATUM(
        jsonb_apply_worker(jb, funcdef, PG_NARGS(), withvarargs, nvarargs, varargs, varnulls, vartypes, fcinfo->
            fncollation));
}
