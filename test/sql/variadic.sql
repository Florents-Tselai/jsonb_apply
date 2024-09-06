select jsonb_apply_variadic('"hello world"', 'replace(text, text, text)', '"hello"', '"bye"');

select jsonb_apply_variadic('"Pg"', 'repeat(text, integer)', 2);

select jsonb_apply_variadic('"abc~@~def~@~ghi"', 'split_part(text,text,integer)', '~@~', 2);
