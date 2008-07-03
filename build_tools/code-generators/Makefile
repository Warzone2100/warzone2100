DB_TEMPLATES=test_code.tpl

C_HEADERS=$(DB_TEMPLATES:%.tpl=%.h)
SQL_TABLES=$(DB_TEMPLATES:%.tpl=%.sql)

all: $(C_HEADERS) $(SQL_TABLES)

%.h: %.tpl
	./db-lang.pl c_structdef $< > $@

%.sql: %.tpl
	./db-lang.pl sql_tabledef $< > $@

clean:
	rm -f $(C_HEADERS) $(SQL_TABLES)

.PHONY: all clean
