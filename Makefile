EXTENSION = jsonb_apply
EXTVERSION = 0.1.0

PG_CONFIG ?= pg_config

EXT_CTRL_FILE = $(EXTENSION).control
PGFILEDESC = $(shell cat $(EXT_CTRL_FILE) | grep 'comment' | sed "s/^.*'\(.*\)'$\/\1/g")
EXT_REQUIRES = $(shell cat $(EXT_CTRL_FILE) | grep 'requires' | sed "s/^.*'\(.*\)'$\/\1/g")
PGVERSION = $(shell $(PG_CONFIG) --version | sed "s/PostgreSQL //g")

MODULE_big = $(EXTENSION)

OBJS = src/$(EXTENSION).o

TESTS = $(wildcard test/sql/notypehints.sql)
ifdef TEST_TYPEHINTS
TESTS += tests/sql/withtypehints.sql
endif
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test --load-extension=$(EXTENSION)

DATA = $(wildcard sql/*--*.sql)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

dev: clean all install installcheck