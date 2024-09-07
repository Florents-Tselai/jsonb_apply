CREATE FUNCTION jsonb_apply(jsonb, text, variadic "any" default null) RETURNS jsonb
AS
'MODULE_PATHNAME'
    LANGUAGE C;
