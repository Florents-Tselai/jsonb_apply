select jsonb_apply_variadic('"hello world"', 'replace(text, text, text)', '"hello"', '"bye"');
