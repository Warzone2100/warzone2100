if PERL_AVAILABLE
%.schema.sql: $(top_srcdir)/build_tools/code-generators/db-lang.pl $(top_srcdir)/build_tools/code-generators/sql_tabledef_cg.pm $(top_srcdir)/src/%.tpl
	$(PERL) $^ $@

%.data.sql: $(top_srcdir)/build_tools/code-generators/db-lang.pl $(top_srcdir)/build_tools/code-generators/sql_insertdef_cg.pm $(srcdir)/../stats $(top_srcdir)/src/%.tpl
	$(PERL) $^ $@

BUILT_SOURCES = \
	stats-db2.data.sql \
	stats-db2.schema.sql
endif
