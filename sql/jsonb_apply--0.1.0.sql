CREATE FUNCTION jsonb_apply(jsonb, text, variadic "any" default null) RETURNS jsonb
AS
'MODULE_PATHNAME',
'jsonb_apply_nofilter'
    LANGUAGE C;

CREATE FUNCTION jsonb_apply(jsonb, regproc, variadic "any" default null) RETURNS jsonb
AS
'MODULE_PATHNAME',
'jsonb_apply_nofilter'
    LANGUAGE C;

CREATE FUNCTION jsonb_filter_apply(jsonb, text[], text, variadic "any" default null) RETURNS jsonb
AS
'MODULE_PATHNAME',
'jsonb_apply_withfilter'
    LANGUAGE C;

CREATE FUNCTION jsonb_filter_apply(jsonb, text[], regproc, variadic "any" default null) RETURNS jsonb
AS
'MODULE_PATHNAME',
'jsonb_apply_withfilter'
    LANGUAGE C;
