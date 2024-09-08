# jsonb_apply: Functional json in Postgres
[![build](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml/badge.svg)](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml)

Have you ever wanted to recursively apply a function to a `jsonb` object?
Like this 
```tsql
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'reverse');
                   jsonb_apply                    
--------------------------------------------------
 {"id": 1, "name": "nhoJ", "messages": ["olleh"]}
(1 row)
```

No? Well, here's a Postgres extension that allows you to do just that:

## Usage

```tsql
select jsonb_apply(doc jsonb, func text[, variadic "any" args1_n]);
```
Currently only functions with signature like `func(text, arg1 "any", ...) → text` are supported.
`arg0` should be `text` and is taken as the `jsonb` string value (or array element) currently being processed.
The `variadic` arguments (if any) will be passed to `func` as `args1...argsn`, while their types will be used to search for the appropriate function in the catalog.

functions applied recursively to string values or elements in `jsonb`.
```tsql
create extension jsonb_apply;
```

```tsql
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'reverse');
                   jsonb_apply                    
--------------------------------------------------
 {"id": 1, "name": "nhoJ", "messages": ["olleh"]}
(1 row)
```
```tsql
select jsonb_apply('{
  "id": 1,
  "name": "  John ",
  "messages": [
    "    hELLo  "
  ]
}', 'btrim');
                   jsonb_apply                    
--------------------------------------------------
 {"id": 1, "name": "John", "messages": ["hELLo"]}
(1 row)
```

```tsql
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'md5');
                                               jsonb_apply                                               
---------------------------------------------------------------------------------------------------------
 {"id": 1, "name": "61409aa1fd47d4a5332de23cbf59a36f", "messages": ["5d41402abc4b2a76b9719d911017c592"]}
(1 row)
```

```tsql
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'repeat', 3);
                            jsonb_apply                             
--------------------------------------------------------------------
 {"id": 1, "name": "JohnJohnJohn", "messages": ["hellohellohello"]}
(1 row)
```

```tsql
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'replace', 'hello', 'bye');
                  jsonb_apply                   
------------------------------------------------
 {"id": 1, "name": "John", "messages": ["bye"]}
(1 row)
```

## Installation

```
cd /tmp
git clone https://github.com/Florents-Tselai/jsonb_apply.git
cd jsonb_apply
make
make install # may need sudo
```

While browsing Postgres' source code, I came across the [`transform_jsonb_string_values`](https://github.com/postgres/postgres/blob/82b07eba9e8b863cc05adb7e53a86ff02b51d888/src/include/utils/jsonfuncs.h#L62) function,
which gave me the idea.
