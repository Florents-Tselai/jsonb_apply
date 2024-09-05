-- func passed without args
select jsonb_apply('{}', 'updfgdper');
select jsonb_apply('{}', 'updfgdper()');

-- sanity checks and errors for functions with not-suport nargs or rettype
select jsonb_apply('"hello"', 'length(text)');
select jsonb_apply('"hello"', 'replace(text, text, text)');

-- basic transformation functions not relying on encoding (e.g. md5 does)
select jsonb_apply('"hELLo"', 'lower(text)');
select jsonb_apply('"hELLo"', 'upper(text)');
select jsonb_apply('"    hELLo  "', 'ltrim(text)');
select jsonb_apply('"    hELLo  "', 'rtrim(text)');
select jsonb_apply('"    hELLo  "', 'btrim(text)');
select jsonb_apply('"hello"', 'quote_ident(text)');
select jsonb_apply('"hello"', 'quote_literal(text)');
select jsonb_apply('"hELLo"', 'reverse(text)');

-- complex objects
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'lower(text)');
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'upper(text)');
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'reverse(text)');


