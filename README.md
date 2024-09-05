# jsonb_apply: Functional json in Postgres
[![build](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml/badge.svg)](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml)

Have you ever wanted to recrusively apply a function to a `jsonb` object?
Like this 
```tsql
select jsonb_apply(jsonb, func);
```
No? Well, here's a Postgres extension that allows you to do just that:

## Installation

```
cd /tmp
git clone https://github.com/Florents-Tselai/jsonb_apply.git
cd jsonb_apply
make
make install # may need sudo
```

## Usage

Currently supports `func(text)->text` functions applied recursively to string values or elements in `jsonb`.
```tsql
create extension jsonb_apply;
```

```sql
select jsonb_apply('{"first": "John", "last": "Doe", "message": "Who are you?"}', 'lower(text)');
                         jsonb_apply                         
-------------------------------------------------------------
 {"last": "doe", "first": "john", "message": "who are you?"}
(1 row)

```

```sql
select jsonb_apply('{"message":  "a secret"}', 'md5(text)');
                   jsonb_apply                   
-------------------------------------------------
 {"message": "8b32185212f8625d1752b360f224fba2"}
(1 row)
```

```sql
select jsonb_apply('{
  "f": "John",
  "l": "Doe",
  "message": "Who are you?",
  "arr": [
    "Hello",
    {
      "k": "value"
    }
  ]
}', 'reverse(text)');
                                      jsonb_apply                                       
----------------------------------------------------------------------------------------
 {"f": "nhoJ", "l": "eoD", "arr": ["olleH", {"k": "eulav"}], "message": "?uoy era ohW"}
(1 row)
```

While browsing Postgres' source code, I came across the [`transform_jsonb_string_values`](https://github.com/postgres/postgres/blob/82b07eba9e8b863cc05adb7e53a86ff02b51d888/src/include/utils/jsonfuncs.h#L62) function.

Which gave me the idea to try something like this:


## Installation
```
``
