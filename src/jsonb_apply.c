#include "postgres.h"

#include "fmgr.h"

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

typedef struct JsonbApplyState {
    FunctionCallInfo top_fcinfo; /* fcinfo from top jsonb_apply(PG_FUNCTION_ARGS)*/
    Datum funcregproc; /* the fucnregproc to be applied */
    Oid funcoid;
    Form_pg_proc procStruct; /*  */
} JsonbApplyState;

static text *
apply_func_jsonb_value(void *_state, char *elem_value, int elem_len) {
    JsonbApplyState *state = (JsonbApplyState *) _state;
    Oid funcoid = state->funcoid;
    Oid collation = state->top_fcinfo->fncollation;
    Datum func_result;
    text *result;

    //    elog(INFO, "collid=%u, funcoid=%u", state->top_fcinfo->fncollation, state->funcoid);

    if (funcoid == 870) {
        result = cstring_to_text(str_tolower(elem_value, elem_len, collation));
    } else if (funcoid == 871) {
        result = cstring_to_text(str_toupper(elem_value, elem_len, collation));
    } else {
        /* Can't figure out why some funcs need a copy. So making them all use copy. */
        char *elemcopy = (char *) malloc(elem_len + 1);
        if (elemcopy != NULL) {
            memcpy(elemcopy, elem_value, elem_len);
            elemcopy[elem_len] = '\0';
        }

        func_result = OidFunctionCall1(funcoid, CStringGetTextDatum(elemcopy));
        result = DatumGetTextPP(func_result);

        free(elemcopy);
    }

    return result;
}


PG_FUNCTION_INFO_V1(jsonb_apply);


Datum
jsonb_apply(PG_FUNCTION_ARGS) {
    /* Input */
    Jsonb *jb = PG_GETARG_JSONB_P(0);
    char *funcdef = text_to_cstring(PG_GETARG_TEXT_PP(1));
    HeapTuple tuple;

    /* Output */
    Jsonb *out;

    /* Action State */
    JsonTransformStringValuesAction action = (JsonTransformStringValuesAction) apply_func_jsonb_value;
    JsonbApplyState *state = palloc0(sizeof(JsonbApplyState));
    state->top_fcinfo = fcinfo;

    /*
     * Search for the function in the system catalog,
     * by trying to create a `regprocedure` out of the `funcdef`
     * The implementation here is copied from to_regprocedure in postgres/.../regproc.c:
     * TODO: Call that instead directly
     * as DirectInputFunctionCallSafe is only available in PG 16, 17
     */

    if (!DirectInputFunctionCallSafe(regprocedurein, funcdef,
                                     InvalidOid, -1,
                                     NULL,
                                     &state->funcregproc))

        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("Invalid function name: %s", funcdef)));

    state->funcoid = DatumGetObjectId(state->funcregproc);

    /*
     * Lets' fill in the state->procStruct which contains info about the function we call (see pg_proc relation)
     */
    tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(state->funcoid));
    if (!HeapTupleIsValid(tuple))
        elog(ERROR, "cache lookup failed for function %u", state->funcoid);
    state->procStruct = (Form_pg_proc) GETSTRUCT(tuple);


    printf("proname=%s\tpronargs=%d\tprorettype=%d\n",
           NameStr(state->procStruct->proname),
           state->procStruct->pronargs,
           state->procStruct->prorettype
    );

    /* Sanity checks */

    if (state->procStruct->pronargs != 1)
        elog(ERROR, "only functions with pronargs=1 are supported, requested function has pronargs=%d",
         state->procStruct->pronargs);
    if (state->procStruct->prorettype != TEXTOID)
        elog(ERROR, "requested function does not return \"text\", but oid=%d", state->procStruct->prorettype);


    out = transform_jsonb_string_values(jb, state, action);

    ReleaseSysCache(tuple);

    pfree(state);
    PG_RETURN_JSONB_P(out);
}

PG_FUNCTION_INFO_V1(jsonb_apply_variadic);


Datum
jsonb_apply_variadic(PG_FUNCTION_ARGS) {
    PG_RETURN_DATUM(DirectFunctionCall1(jsonb_in, CStringGetDatum("\"bye world\"")));
}
