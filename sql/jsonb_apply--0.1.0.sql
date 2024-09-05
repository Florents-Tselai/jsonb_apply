CREATE FUNCTION jsonb_apply(jsonb, text) RETURNS jsonb
AS
'MODULE_PATHNAME'
    LANGUAGE C;

CREATE FUNCTION jsonb_apply_variadic(jsonb, text, variadic params jsonb[]) RETURNS jsonb
AS
'MODULE_PATHNAME'
    LANGUAGE C;
