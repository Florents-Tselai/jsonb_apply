select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'reverse');
select jsonb_apply('{
  "id": 1,
  "name": "  John ",
  "messages": [
    "    hELLo  "
  ]
}', 'btrim');
select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'md5');

select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}'::jsonb #> '{messages}', 'md5');

select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'repeat', 3);

select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'replace', 'hello', 'bye');
