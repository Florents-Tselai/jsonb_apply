-- error messages
select jsonb_apply('"hello"', 'length(text)');
select jsonb_apply('"hello"', 'gsdgfd(text, text)');

create function lorem() returns text language sql as $$ select 'lorem ipsum'$$;

-- funcs with nargs=0
select jsonb_apply('"hello"', 'lorem()');

-- funcs with nargs=1
select jsonb_apply('"hello"', 'lower(text)');
select jsonb_apply('"hello"', 'upper(text)');
select jsonb_apply('"hello"', 'reverse(text)');
select jsonb_apply('"    hELLo  "', 'ltrim(text)');
select jsonb_apply('"    hELLo  "', 'rtrim(text)');
select jsonb_apply('"    hELLo  "', 'btrim(text)');

-- funcs with combination of argtypes
select jsonb_apply('"hello world"', 'replace(text, text, text)', 'hello', 'bye');
select jsonb_apply('"Pg"', 'repeat(text, integer)', 3);
select jsonb_apply('"abc~@~def~@~ghi"', 'split_part(text,text,integer)', '~@~', 2);


-- complex object, just checking for spurious pointers.
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'lower(text)');
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'upper(text)');
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'reverse(text)');
