%.out: %.sql
	psql -d postgres -ef $< > $@

all: examples.out

clean:
	-rm *.out