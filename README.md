# jsonb_apply: Functional json for postgres
[![build](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml/badge.svg)](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml)

Exploring whether something like this is useful:

```tsql
select jsonb_apply(jsonb, func)
```

Currently supports `func(text)->text` functions applied recursively to string values in `jsonb`

```sql
select jsonb_apply('{"first": "John", "last": "Doe", "message": "Who are you?"}', 'lower(text)');
                         jsonb_apply                         
-------------------------------------------------------------
 {"last": "doe", "first": "john", "message": "who are you?"}
(1 row)

```

```sql
select jsonb_apply('"hello"', 'md5(text)');
            jsonb_apply             
------------------------------------
 "5d41402abc4b2a76b9719d911017c592"
(1 row)
```

```sql
select jsonb_apply('"hELLo"', 'reverse(text)');
 jsonb_apply 
-------------
 "oLLEh"
(1 row)
```
