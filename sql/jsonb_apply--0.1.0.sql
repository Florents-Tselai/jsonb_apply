CREATE FUNCTION jsonb_apply(jsonb, text) RETURNS jsonb
AS
'MODULE_PATHNAME'
    LANGUAGE C;

CREATE FUNCTION jsonb_apply_variadic(jsonb, text, variadic "any" default null) RETURNS jsonb
AS
'MODULE_PATHNAME'
    LANGUAGE C;
