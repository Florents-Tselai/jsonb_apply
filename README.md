# jsonb_apply: Postgres JSON with a functional twist

[![build](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml/badge.svg)](https://github.com/Florents-Tselai/jsonb_apply/actions/workflows/build.yml)

Have you ever wanted to apply a function to a `jsonb` object, both dynamically and recursively?

No? Well, here's a Postgres extension that allows you to do just that:

```tsql
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'upper');
```
```tsql
                   jsonb_apply                    
--------------------------------------------------
 {"id": 1, "name": "JOHN", "messages": ["HELLO"]}
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
```
```tsql
                  jsonb_apply                   
------------------------------------------------
 {"id": 1, "name": "John", "messages": ["bye"]}
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
```
```tsql
                   jsonb_apply                    
--------------------------------------------------
 {"id": 1, "name": "John", "messages": ["hELLo"]}
(1 row)
```


```tsql
select jsonb_apply('"abc~@~def~@~ghi"', 'split_part', '~@~', 2);
```
```tsql
 jsonb_apply 
-------------
 "def"
(1 row)
```

```tsql
select jsonb_filter_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', '{messages}', 'md5');
```
```tsql
          jsonb_filter_apply          
--------------------------------------
 ["5d41402abc4b2a76b9719d911017c592"]
(1 row)
```

## Usage

```tsql
select jsonb_apply(doc jsonb, func text[, variadic "any" args1_n]);
```

There's also a variant that applies the function to a sub-object of the original document.
See [json_extract_path](https://pgpedia.info/j/json_extract_path.html).

```tsql
select jsonb_filter_apply(doc jsonb, filter text[] text[, variadic "any" args1_n]);
```

Currently only functions with signature like `func(text, arg1 "any", ...) → text` are supported.
The first argument `arg0` should be `text` and is taken as the `jsonb` string value (or array element) currently being processed.
The `variadic` arguments (if any) will be passed to `func` as `args1...argsn`.
Bot the function name and the types of `variadic` arguments will be used to search for the appropriate function in the catalog.

## Installation

```
cd /tmp
git clone https://github.com/Florents-Tselai/jsonb_apply.git
cd jsonb_apply
make
make install # may need sudo
```

```tsql
create extension jsonb_apply;
```
