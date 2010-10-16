%.tab.h %.tab.c:: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.h %.lex.c:: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<
#	sed -i 's/\(fwrite(.*)\)/abs(\1)/' $@

# Required to force using a C++ linker
force-linker.cpp:
	touch $@

AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(OGGVORBIS_CFLAGS) $(OPENAL_CFLAGS) $(OPENGLC_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) $(GLee_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)
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

CLEANFILES = \
	force-linker.cpp \
	level_lexer.lex.c \
	message_lexer.lex.c \
	message_parser.tab.c \
	message_parser.tab.h \
	scriptvals_lexer.lex.c \
	scriptvals_parser.tab.c \
	scriptvals_parser.tab.h

EXTRA_DIST = \
	message_lexer.l message_parser.y \
	scriptvals_lexer.l scriptvals_parser.y \
	level_lexer.l

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
	challenge.h \
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
	modding.h \
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
	pointtree.h \
	positiondef.h \
	power.h \
	projectiledef.h \
	projectile.h \
	radar.h \
	random.h \
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
	statsdef.h \
	stats.h \
	stringdef.h \
	structuredef.h \
	structure.h \
	terrain.h \
	text.h \
	texture.h \
	transporter.h \
	visibility.h \
	version.h \
	warcam.h \
	warzoneconfig.h \
	wavecast.h \
	weapondef.h \
	wrappers.h

warzone2100_SOURCES = \
	action.c \
	advvis.c \
	ai.c \
	aiexperience.c \
	astar.cpp \
	atmos.c \
	aud.c \
	baseobject.c \
	bridge.c \
	bucket3d.c \
	challenge.c \
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
	mapgrid.cpp \
	mechanics.c \
	message.c \
	message_lexer.lex.c \
	message_parser.tab.c \
	miscimd.c \
	mission.c \
	move.c \
	multibot.cpp \
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
	pointtree.cpp \
	power.c \
	projectile.c \
	radar.c \
	random.cpp \
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
	terrain.c \
	text.c \
	texture.c \
	transporter.c \
	version.c \
	visibility.c \
	warcam.c \
	warzoneconfig.c \
	wavecast.cpp \
	wrappers.c

warzone2100_LDADD = \
	$(top_builddir)/lib/widget/libwidget.a \
	$(top_builddir)/lib/sequence/libsequence.a \
	$(top_builddir)/lib/sound/libsound.a \
	$(top_builddir)/lib/script/libscript.a \
	$(top_builddir)/lib/netplay/libnetplay.a \
	$(top_builddir)/lib/netplay/miniupnpc/libminiupnpc.a \
	$(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/ivis_common/libivis_common.a \
	$(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a \
	$(top_builddir)/lib/iniparser/libiniparser.a \
	$(top_builddir)/lib/exceptionhandler/libexceptionhandler.a

warzone2100_LDADD += $(LTLIBINTL) $(SDL_LIBS) $(PHYSFS_LIBS) $(PNG_LIBS) $(OGGVORBIS_LIBS) $(THEORA_LIBS) $(OPENAL_LIBS) $(OPENGLC_LIBS) $(OPENGL_LIBS) $(X11_LIBS) $(GLee_LIBS)

if MINGW32
warzone2100_LDADD += $(top_builddir)/win32/warzone2100.o $(WIN32_LIBS)
endif

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
