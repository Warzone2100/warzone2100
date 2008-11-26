DB_TEMPLATES=test_code.tpl

C_SOURCES=$(DB_TEMPLATES:%.tpl=%.c)
C_HEADERS=$(DB_TEMPLATES:%.tpl=%.h)
SQL_TABLES=$(DB_TEMPLATES:%.tpl=%.sql)

all: $(C_SOURCES) $(C_HEADERS) $(SQL_TABLES)

%.c: ./db-lang.pl c_sqliteload_cg.pm %.tpl.sql.c %.tpl
	./$^ $@

%.h: ./db-lang.pl c_structdef_cg.pm %.tpl.struct.h %.tpl
	./$^ $@

%.sql: ./db-lang.pl sql_tabledef_cg.pm %.tpl
	./$^ $@

clean:
	rm -f $(C_SOURCES) $(C_HEADERS) $(SQL_TABLES)

.PHONY: all clean
