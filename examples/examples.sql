select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'upper');

select jsonb_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', 'replace', 'hello', 'bye');

select jsonb_apply('{
  "id": 1,
  "name": "  John ",
  "messages": [
    "    hELLo  "
  ]
}', 'btrim');

select jsonb_apply('"abc~@~def~@~ghi"', 'split_part', '~@~', 2);

select jsonb_filter_apply('{
  "id": 1,
  "name": "John",
  "messages": [
    "hello"
  ]
}', '{messages}', 'md5');
