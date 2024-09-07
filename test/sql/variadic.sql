create function lorem() returns text language sql as $$ select 'lorem ipsum'$$;

select jsonb_apply_variadic('"hello"', 'lorem()');
select jsonb_apply_variadic('"hello"', 'lower(text)');
select jsonb_apply_variadic('"hello"', 'upper(text)');
select jsonb_apply_variadic('"hello"', 'reverse(text)');

select jsonb_apply_variadic('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'lower(text)');
select jsonb_apply_variadic('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'upper(text)');
select jsonb_apply_variadic('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'reverse(text)');


select jsonb_apply_variadic('"hello world"', 'replace(text, text, text)', 'hello', 'bye');

select jsonb_apply_variadic('"Pg"', 'repeat(text, integer)', 3);

select jsonb_apply_variadic('"abc~@~def~@~ghi"', 'split_part(text,text,integer)', '~@~', 2);
