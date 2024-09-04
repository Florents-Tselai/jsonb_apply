-- func passed without args
select jsonb_apply('{}', 'updfgdper');
select jsonb_apply('{}', 'updfgdper()');

select jsonb_apply('"hELLo"', 'lower(text)');
select jsonb_apply('"hELLo"', 'upper(text)');

select jsonb_apply('{"a": "aaa", "b":"B", "c": 1, "d": "hELLo world" }', 'lower(text)');
select jsonb_apply('{"a": "aaa", "b":"B", "c": 1, "d": "hELLo world" }', 'upper(text)');

-- some other func(text) -> text functions
select jsonb_apply('"hello"', 'md5(text)');
select jsonb_apply('"hello"', 'quote_ident(text)');
select jsonb_apply('"hello"', 'quote_literal(text)');

-- sanity checks and errors
select jsonb_apply('"hello"', 'length(text)');
select jsonb_apply('"hello"', 'replace(text, text, text)');

-- select proname,
--        prosrc,
--        proargtypes,
--        prorettype,
--        'select ' || 'jsonb_apply(''"hello"'', ' || '''' || proname || '(text)'');'
-- select pg_proc
--     where proargtypes = '25'::oidvector
--   and prorettype = 25
-- order by proname;


select jsonb_apply('"hELLo"', 'btrim(text)');


select jsonb_apply('"hELLo"', 'current_setting(text)');


select jsonb_apply('"hELLo"', 'format(text)');


select jsonb_apply('"hELLo"', 'initcap(text)');


select jsonb_apply('"hELLo"', 'lower(text)');



select jsonb_apply('"hELLo"', 'ltrim(text)');



select jsonb_apply('"hELLo"', 'max(text)');


select jsonb_apply('"hELLo"', 'md5(text)');


select jsonb_apply('"hELLo"', 'min(text)');


select jsonb_apply('"hELLo"', 'pg_current_logfile(text)');


select jsonb_apply('"hELLo"', 'pg_get_viewdef(text)');


select jsonb_apply('"hELLo"', 'pg_ls_dir(text)');


select jsonb_apply('"hELLo"', 'pg_read_file(text)');


select jsonb_apply('"hELLo"', 'quote_ident(text)');


select jsonb_apply('"hELLo"', 'quote_literal(text)');


select jsonb_apply('"hELLo"', 'quote_nullable(text)');


select jsonb_apply('"hELLo"', 'reverse(text)');


select jsonb_apply('"hELLo"', 'rtrim(text)');


select jsonb_apply('"hELLo"', 'similar_to_escape(text)');


select jsonb_apply('"hELLo"', 'to_ascii(text)');


select jsonb_apply('"hELLo"', 'unistr(text)');


select jsonb_apply('"hELLo"', 'upper(text)');
