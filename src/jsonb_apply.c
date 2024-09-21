#include "postgres.h"

#include "fmgr.h"
#include "funcapi.h"
#include "utils/builtins.h"
#include "utils/jsonb.h"
#include "utils/jsonfuncs.h" /* transform_jsonb_string_values */
#include "catalog/pg_proc.h"
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

/* Declarations */
static text *variadic_apply_func_jsonb_value(void *_state, char *elem_value, int elem_len);

Datum jsonb_apply_internal(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_apply);

typedef struct JsonbApplyState {
    Oid collation;
    FmgrInfo *fnFmgrInfo;
    const Datum *funcargs1_n; /* arg0 is the json string value itself, additional are supplied by the user. */
    const Oid *funcargs1_n_types;
} JsonbApplyState;

#define FUNC_OID(f) (DatumGetObjectId((f)->proc))

static text *
variadic_apply_func_jsonb_value(void *_state, char *elem_value, int elem_len) {
    JsonbApplyState *state = (JsonbApplyState *) _state;
    int fn_nargs = state->fnFmgrInfo->fn_nargs;
    PGFunction fn = state->fnFmgrInfo->fn_addr;
    Datum arg0;
    const Datum *args1_n = state->funcargs1_n;
    Oid collation = state->collation;

    Datum result;

    /* Can't figure out why some funcs need a copy. So making them all use copy. */
    char *elemcopy = (char *) malloc(elem_len + 1);
    if (elemcopy != NULL) {
        memcpy(elemcopy, elem_value, elem_len);
        elemcopy[elem_len] = '\0';
    }
    arg0 = CStringGetTextDatum(elemcopy);

    switch (fn_nargs) {
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

/* Implementation for jsonb_apply(jsonb, text, variadic "any" default null) */
Datum jsonb_apply_internal(FunctionCallInfo fcinfo) {
    /* Input */
    Jsonb *jb = PG_GETARG_JSONB_P(0); /* That's always the first argument */
    Datum fnKey = PG_GETARG_DATUM(1); /* The function the user says wants to apply, we toggle on its Oid later*/
    Oid fnKeyTypeOid = get_fn_expr_argtype(fcinfo->flinfo, 1);
    int variadic_start = 2;
    bool variadic_null = PG_NARGS() == 3 && PG_ARGISNULL(2); /* Are we in a variadic = null case ? */

    Datum *varargs;
    bool *varnulls;
    Oid *vartypes;

    /* Function metadata */
    Oid fnOid; /* The Oid we'll be looking for: oid of the function the user actually wants */
    HeapTuple tuple;
    Form_pg_proc fnForm; /* how fn appears in the pg_proc relation*/

    JsonbApplyState *state;
    JsonTransformStringValuesAction action;

    /* Output */
    Jsonb *out;

    /* extract variadic args and their metadata */
    int nvarargs = extract_variadic_args(fcinfo, variadic_start, true, &varargs, &vartypes, &varnulls);

    bool withvarargs = (nvarargs == -1 || variadic_null) ? false : true;

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
    switch ((fnKeyTypeOid)) {
        case TEXTOID:
            fnOid = LookupFuncName(list_make1(makeString(text_to_cstring(DatumGetTextP(fnKey)))),
                                   lookup_nargs,
                                   lookup_argtypes,
                                   false);

            break;
        default:
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("to specify a function you can only use its name")));
    }

    elog(DEBUG1, "fnKeyTypeOid=%d\tfnOid=%d\tlookup_nargs=%d\n", fnKeyTypeOid, fnOid, lookup_nargs);

    tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(fnOid));
    if (!HeapTupleIsValid(tuple))
        elog(ERROR, "cache lookup failed for function %u", fnOid);
    fnForm = (Form_pg_proc) GETSTRUCT(tuple);
    ReleaseSysCache(tuple);

    if ((fnForm->pronargs < 1) || (fnForm->pronargs > 9)) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("functions with at least 1 and more than 9 arguments are not supported")));
    }

    if ((fnForm->prorettype != TEXTOID)) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("only functions that return text are supported")));
    }


    state = palloc0(sizeof(JsonbApplyState));
    state->collation = PG_GET_COLLATION();
    state->fnFmgrInfo = palloc0(sizeof(FmgrInfo));
    fmgr_info(fnOid, state->fnFmgrInfo);

    /* without varargs */
    if (!withvarargs) {
        state->funcargs1_n = NULL;
        state->funcargs1_n_types = NULL;
        /* withvarargs */
    } else {
        state->funcargs1_n = varargs;
        state->funcargs1_n_types = vartypes;
    }

    action = (JsonTransformStringValuesAction) variadic_apply_func_jsonb_value;

    out = transform_jsonb_string_values(jb, state, action);

    pfree(state->fnFmgrInfo);
    pfree(state);

    PG_RETURN_JSONB_P(out);
}

Datum
jsonb_apply(PG_FUNCTION_ARGS) {
    return jsonb_apply_internal(fcinfo);
}
