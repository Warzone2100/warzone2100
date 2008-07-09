DB_TEMPLATES=test_code.tpl

C_SOURCES=$(DB_TEMPLATES:%.tpl=%.c)
C_HEADERS=$(DB_TEMPLATES:%.tpl=%.h)
SQL_TABLES=$(DB_TEMPLATES:%.tpl=%.sql)

all: $(C_SOURCES) $(C_HEADERS) $(SQL_TABLES)

%.c: %.tpl.sql.c %.tpl
	./db-lang.pl c_sqliteload $^ > $@

%.h: %.tpl
	./db-lang.pl c_structdef $< > $@

%.sql: %.tpl
	./db-lang.pl sql_tabledef $< > $@

clean:
	rm -f $(C_SOURCES) $(C_HEADERS) $(SQL_TABLES)

.PHONY: all clean
