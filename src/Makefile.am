%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.h %.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

if PERL_AVAILABLE
%.c: $(top_srcdir)/build_tools/code-generators/db-lang.pl $(top_srcdir)/build_tools/code-generators/c_sqliteload_cg.pm %.tpl.sql.c %.tpl
	$(PERL) $^ $@

%.h: $(top_srcdir)/build_tools/code-generators/db-lang.pl $(top_srcdir)/build_tools/code-generators/c_structdef_cg.pm %.tpl.struct.h %.tpl
	$(PERL) $^ $@
endif

# Required to force using a C++ linker
force-linker.cpp:
	touch $@

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(SDL_NET_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OGGVORBIS_CFLAGS) $(OPENAL_CFLAGS) $(OPENGLC_CFLAGS) $(OPENGL_CFLAGS) $(POPT_CFLAGS) $(SQLITE_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d

BUILT_SOURCES = \
	force-linker.cpp \
	level_lexer.lex.c \
	message_lexer.lex.c \
	message_parser.tab.c \
	message_parser.tab.h \
	scriptvals_lexer.lex.c \
	scriptvals_parser.tab.c \
	scriptvals_parser.tab.h

if PERL_AVAILABLE
BUILT_SOURCES += \
	stats-db2.c \
	stats-db2.h
endif

CLEANFILES = \
	force-linker.cpp \
	level_lexer.lex.c \
	message_lexer.lex.c \
	message_parser.tab.c \
	message_parser.tab.h \
	scriptvals_lexer.lex.c \
	scriptvals_parser.tab.c \
	scriptvals_parser.tab.h

bin_PROGRAMS = warzone2100
noinst_HEADERS = \
	action.h \
	advvis.h \
	aiexperience.h \
	ai.h \
	anim_id.h \
	astar.h \
	atmos.h \
	basedef.h \
	baseobject.h \
	bridge.h \
	bucket3d.h \
	cheat.h \
	clparse.h \
	cluster.h \
	cmddroiddef.h \
	cmddroid.h \
	combat.h \
	component.h \
	configuration.h \
	console.h \
	data.h \
	design.h \
	difficulty.h \
	display3ddef.h \
	display3d.h \
	displaydef.h \
	display.h \
	drive.h \
	droiddef.h \
	droid.h \
	e3demo.h \
	edit3d.h \
	effects.h \
	environ.h \
	featuredef.h \
	feature.h \
	formationdef.h \
	formation.h \
	fpath.h \
	frend.h \
	frontend.h \
	functiondef.h \
	function.h \
	game.h \
	gateway.h \
	geometry.h \
	group.h \
	hci.h \
	ingameop.h \
	init.h \
	intdisplay.h \
	intelmap.h \
	intfac.h \
	intimage.h \
	intorder.h \
	keybind.h \
	keyedit.h \
	keymap.h \
	levelint.h \
	levels.h \
	lighting.h \
	loadsave.h \
	loop.h \
	main.h \
	mapdisplay.h \
	mapgrid.h \
	map.h \
	mechanics.h \
	messagedef.h \
	message.h \
	messagely.h \
	message_parser.tab.h \
	miscimd.h \
	missiondef.h \
	mission.h \
	movedef.h \
	move.h \
	multigifts.h \
	multiint.h \
	multijoin.h \
	multilimit.h \
	multimenu.h \
	multiplay.h \
	multirecv.h \
	multistat.h \
	objectdef.h \
	objects.h \
	objmem.h \
	oprint.h \
	orderdef.h \
	order.h \
	parsetest.h \
	positiondef.h \
	power.h \
	projectiledef.h \
	projectile.h \
	radar.h \
	raycast.h \
	researchdef.h \
	research.h \
	scores.h \
	scriptai.h \
	scriptcb.h \
	scriptextern.h \
	scriptfuncs.h \
	scriptobj.h \
	scripttabs.h \
	scriptvals.h \
	scriptvals_parser.tab.h \
	selection.h \
	seqdisp.h \
	stats-db2.h \
	statsdef.h \
	stats.h \
	structuredef.h \
	structure.h \
	target.h \
	terrain.h \
	text.h \
	texture.h \
	transporter.h \
	visibility.h \
	version.h \
	warcam.h \
	warzoneconfig.h \
	weapondef.h \
	wrappers.h

warzone2100_SOURCES = \
	action.c \
	advvis.c \
	ai.c \
	aiexperience.c \
	astar.c \
	atmos.c \
	aud.c \
	baseobject.c \
	bridge.c \
	bucket3d.c \
	cheat.c \
	clparse.c \
	cluster.c \
	cmddroid.c \
	combat.c \
	component.c \
	configuration.c \
	console.c \
	data.c \
	design.c \
	difficulty.c \
	display3d.c \
	display.c \
	drive.c \
	droid.c \
	e3demo.c \
	edit3d.c \
	effects.c \
	environ.c \
	feature.c \
	force-linker.cpp \
	formation.c \
	fpath.c \
	frontend.c \
	function.c \
	game.c \
	gateway.c \
	geometry.c \
	group.c \
	hci.c \
	ingameop.c \
	init.c \
	intdisplay.c \
	intelmap.c \
	intimage.c \
	intorder.c \
	keybind.c \
	keyedit.c \
	keymap.c \
	level_lexer.lex.c \
	levels.c \
	lighting.c \
	loadsave.c \
	loop.c \
	main.c \
	map.c \
	mapdisplay.c \
	mapgrid.c \
	mechanics.c \
	message.c \
	message_lexer.lex.c \
	message_parser.tab.c \
	miscimd.c \
	mission.c \
	move.c \
	multibot.c \
	multigifts.c \
	multiint.c \
	multijoin.c \
	multilimit.c \
	multimenu.c \
	multiopt.c \
	multiplay.c \
	multistat.c \
	multistruct.c \
	multisync.c \
	objects.c \
	objmem.c \
	oprint.c \
	order.c \
	parsetest.c \
	power.c \
	projectile.c \
	radar.c \
	raycast.c \
	research.c \
	scores.c \
	scriptai.c \
	scriptcb.c \
	scriptextern.c \
	scriptfuncs.c \
	scriptobj.c \
	scripttabs.c \
	scriptvals.c \
	scriptvals_lexer.lex.c \
	scriptvals_parser.tab.c \
	selection.c \
	seqdisp.c \
	stats.c \
	structure.c \
	target.c \
	terrain.c \
	text.c \
	texture.c \
	transporter.c \
	version.c \
	visibility.c \
	warcam.c \
	warzoneconfig.c \
	wrappers.c

warzone2100_LDADD = \
	$(top_builddir)/lib/widget/libwidget.a \
	$(top_builddir)/lib/sequence/libsequence.a \
	$(top_builddir)/lib/sound/libsound.a \
	$(top_builddir)/lib/script/libscript.a \
	$(top_builddir)/lib/netplay/libnetplay.a \
	$(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/ivis_common/libivis_common.a \
	$(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a \
	$(top_builddir)/lib/exceptionhandler/libexceptionhandler.a

warzone2100_LDADD += $(SDL_LIBS) $(SDL_NET_LIBS) $(PHYSFS_LIBS) $(PNG_LIBS) $(OGGVORBIS_LIBS) $(THEORA_LIBS) $(OPENAL_LIBS) $(OPENGLC_LIBS) $(OPENGL_LIBS) $(POPT_LIBS) $(SQLITE_LIBS)

if MINGW32
warzone2100_LDADD += $(top_builddir)/win32/warzone2100.o $(WIN32_LIBS)
endif
