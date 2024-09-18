-- error messages
select jsonb_apply('"hello"', 'length');
select jsonb_apply('"hello"', 'gsdgfd');

create function lorem() returns text language sql as $$ select 'lorem ipsum'$$;

-- funcs with nargs=0
select jsonb_apply('"hello"', 'lorem()');

-- funcs with nargs=1
select jsonb_apply('"hello"', 'lower');
select jsonb_apply('"hello"', 'upper');
select jsonb_apply('"hello"', 'reverse');
select jsonb_apply('"    hELLo  "', 'ltrim');
select jsonb_apply('"    hELLo  "', 'rtrim');
select jsonb_apply('"    hELLo  "', 'btrim');

-- funcs with combination of argtypes
select jsonb_apply('"hello world"', 'replace', 'hello', 'bye');
select jsonb_apply('"Pg"', 'repeat', 3);
select jsonb_apply('"abc~@~def~@~ghi"', 'split_part', '~@~', 2);

-- complex object, just checking for spurious pointers.
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'lower');
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'upper');
select jsonb_apply('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'reverse');

-- filter_apply
select jsonb_filter_apply('{"f": "John", "arr": ["Hello", {"k": "value"}]}', '{arr}' , 'reverse');
select jsonb_filter_apply('{"f": "John", "arr": ["Hello", {"k": "value"}]}', '{arr}' , 'replace', 'Hello', 'Bye');
