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
    Form_pg_proc procStruct; /*  */
    Datum *funcargs1_n; /* arg0 is the json value itself, additional are supplied by the user. */
    Oid *funcargs1_n_types;
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
jsonb_apply_worker(Jsonb *jb, char *funcdef, int nvarargs, const Datum *varargs, const bool *varnulls, const Oid *vartypes, Oid collation) {

    Jsonb *out;

    HeapTuple tuple;

    Func *f = palloc0(sizeof(Func));
    Datum *funcargs1_n = NULL;
    Oid *funcargs1_n_types = NULL;


    /* Search for the function in the catalog and fill-in the Func struct */
    {
        f->indef = DatumGetTextPP(varargs[1]);
        if (!DirectInputFunctionCallSafe(regprocedurein, funcdef,
                                         InvalidOid, -1,
                                         NULL,
                                         &f->proc))

            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                            errmsg("Invalid function name: %s", funcdef)));


        /* Lets' fill in the state->procStruct which contains info about the function we call (see pg_proc relation) */
        tuple = SearchSysCache1(PROCOID, f->proc);
        if (!HeapTupleIsValid(tuple))
            elog(ERROR, "cache lookup failed for function %u", FUNC_OID(f));
        f->form = (Form_pg_proc) GETSTRUCT(tuple);
        ReleaseSysCache(tuple);

        f->finfo = palloc0(sizeof(FmgrInfo));
        fmgr_info(FUNC_OID(f), f->finfo);

    }

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

    /* Fill-in the Datum *funcargs1_n. Arguments to call the function with */
    {
        if (f->form->pronargs <= 1) {
            funcargs1_n = NULL;
            funcargs1_n_types = NULL;
        } else {
            /*TODO: use pointer arithmetic for this */
            funcargs1_n = palloc0(sizeof(Datum) * nvarargs);
            funcargs1_n_types = palloc0(sizeof(Oid) * nvarargs);

            for (int i = 0; i < nvarargs - 1; i++) {
                funcargs1_n[i] = varargs[i];
                funcargs1_n_types[i] = vartypes[i];
            }
        }
    }

    /* fill in transformation state */
    JsonbApplyState *state = palloc0(sizeof(JsonbApplyState));
    state->f = f;
    state->funcargs1_n = funcargs1_n;
    state->funcargs1_n_types = funcargs1_n_types;
    state->fncollation = collation;
    JsonTransformStringValuesAction action = (JsonTransformStringValuesAction) variadic_apply_func_jsonb_value;

    out = transform_jsonb_string_values(jb, state, action);

    /* Cleanup */
    if (funcargs1_n)
        pfree(funcargs1_n);

    if (funcargs1_n_types)
        pfree(funcargs1_n_types);

    pfree(state);
    pfree(f->finfo);
    pfree(f);


    PG_RETURN_JSONB_P(out);
}

PG_FUNCTION_INFO_V1(jsonb_apply);

Datum
jsonb_apply(PG_FUNCTION_ARGS) {
    Jsonb *jb = PG_GETARG_JSONB_P(0);
    char *funcdef = text_to_cstring(PG_GETARG_TEXT_PP(1));
    Datum *varargs;
    bool *varnulls;
    Oid *vartypes;

    /* build argument values to build the object */
    int nvarargs = extract_variadic_args(fcinfo, 0, true,
                                         &varargs, &vartypes, &varnulls);

    if (nvarargs < 0)
        PG_RETURN_NULL();

    PG_RETURN_DATUM(jsonb_apply_worker(jb, funcdef, nvarargs, varargs, varnulls, vartypes, fcinfo->fncollation));
}
