%.schema.sql: $(top_srcdir)/build_tools/code-generators/db-lang.pl $(top_srcdir)/build_tools/code-generators/sql_tabledef_cg.pm $(top_srcdir)/src/%.tpl
	$^ $@

BUILT_SOURCES = \
	stats-db2.schema.sql
