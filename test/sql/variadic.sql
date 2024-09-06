select jsonb_apply_variadic('"hello"', 'lower(text)', NULL);
select jsonb_apply_variadic('"hello"', 'upper(text)', NULL);
select jsonb_apply_variadic('"hello"', 'reverse(text)', NULL);

select jsonb_apply_variadic('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'lower(text)', NULL);
select jsonb_apply_variadic('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'upper(text)', NULL);
select jsonb_apply_variadic('{"f": "John", "l": "Doe", "message": "Who are you?", "arr": ["Hello", {"k": "value"}]}', 'reverse(text)', NULL);


select jsonb_apply_variadic('"hello world"', 'replace(text, text, text)', 'hello', 'bye');

select jsonb_apply_variadic('"Pg"', 'repeat(text, integer)', 3);

select jsonb_apply_variadic('"abc~@~def~@~ghi"', 'split_part(text,text,integer)', '~@~', 2);
