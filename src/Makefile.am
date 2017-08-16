AM_CPPFLAGS = -DYY_NO_INPUT $(SDL_CFLAGS) $(PHYSFS_CFLAGS) $(PNG_CFLAGS) $(VORBISFILE_CFLAGS) $(VORBIS_CFLAGS) $(OPENAL_CFLAGS) $(FONT_CFLAGS) $(OPENGL_CFLAGS) $(WZ_CPPFLAGS) $(GLEW_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT5_CFLAGS)

# Here, we need to set "plugins" for Qt5 to be static
# FIXME: At a future date, we should check if they want static builds, but, since we only support those for now...
if MINGW32
AM_CFLAGS += -DQT_STATICPLUGIN
AM_CPPFLAGS += -DQT_STATICPLUGIN
AM_CXXFLAGS += -DQT_STATICPLUGIN
endif

MOCHEADER = \
	multiint.h \
	qtscriptdebug.h
MOCEDFILES = $(MOCHEADER:%.h=%_moc.cpp)

%_moc.cpp: %.h
	$(MOC5) -o $@ $<

.PHONY: autorevision.h

autorevision.h autorevision.cache:
	cd $(top_srcdir) && ./build_tools/autorevision -t h -o "$(abs_builddir)/autorevision.cache" > "$(abs_builddir)/autorevision.h.new"
	cmp -s autorevision.h.new autorevision.h && rm autorevision.h.new || mv -f autorevision.h.new autorevision.h

CLEANFILES = \
	$(BUILT_SOURCES) \
	autorevision.h.new \
	$(MOCEDFILES)

BUILT_SOURCES = \
	autorevision.h

EXTRA_DIST = \
	autorevision.cache \
	cursorselection

if PORTABLE
bin_PROGRAMS = warzone2100_portable
else
bin_PROGRAMS = warzone2100
endif

noinst_HEADERS = \
	action.h \
	actiondef.h \
	advvis.h \
	ai.h \
	astar.h \
	atmos.h \
	basedef.h \
	baseobject.h \
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
	droiddef.h \
	droid.h \
	edit3d.h \
	effects.h \
	featuredef.h \
	feature.h \
	fpath.h \
	frend.h \
	frontend.h \
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
	pointtree.h \
	positiondef.h \
	power.h \
	projectiledef.h \
	projectile.h \
	qtscript.h \
	qtscriptfuncs.h \
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
	scriptvals_parser.h \
	selection.h \
	seqdisp.h \
	statsdef.h \
	stats.h \
	stringdef.h \
	structuredef.h \
	structure.h \
	template.h \
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
	wrappers.h \
	$(MOCHEADER)

nodist_COMMONSOURCES = \
	multiint_moc.cpp \
	qtscriptdebug_moc.cpp

COMMONSOURCES = \
	action.cpp \
	advvis.cpp \
	ai.cpp \
	astar.cpp \
	atmos.cpp \
	aud.cpp \
	baseobject.cpp \
	bucket3d.cpp \
	challenge.cpp \
	cheat.cpp \
	clparse.cpp \
	cluster.cpp \
	cmddroid.cpp \
	combat.cpp \
	component.cpp \
	configuration.cpp \
	console.cpp \
	data.cpp \
	design.cpp \
	difficulty.cpp \
	display3d.cpp \
	display.cpp \
	droid.cpp \
	edit3d.cpp \
	effects.cpp \
	feature.cpp \
	fpath.cpp \
	frontend.cpp \
	game.cpp \
	gateway.cpp \
	geometry.cpp \
	group.cpp \
	hci.cpp \
	ingameop.cpp \
	init.cpp \
	intdisplay.cpp \
	intelmap.cpp \
	intimage.cpp \
	intorder.cpp \
	keybind.cpp \
	keyedit.cpp \
	keymap.cpp \
	level_lexer.cpp \
	levels.cpp \
	lighting.cpp \
	loadsave.cpp \
	loop.cpp \
	main.cpp \
	map.cpp \
	mapdisplay.cpp \
	mapgrid.cpp \
	mechanics.cpp \
	message.cpp \
	miscimd.cpp \
	mission.cpp \
	modding.cpp \
	move.cpp \
	multibot.cpp \
	multigifts.cpp \
	multiint.cpp \
	multijoin.cpp \
	multilimit.cpp \
	multimenu.cpp \
	multiopt.cpp \
	multiplay.cpp \
	multistat.cpp \
	multistruct.cpp \
	multisync.cpp \
	objects.cpp \
	objmem.cpp \
	oprint.cpp \
	order.cpp \
	pointtree.cpp \
	power.cpp \
	projectile.cpp \
	qtscript.cpp \
	qtscriptdebug.cpp \
	qtscriptfuncs.cpp \
	radar.cpp \
	random.cpp \
	raycast.cpp \
	research.cpp \
	scores.cpp \
	scriptai.cpp \
	scriptcb.cpp \
	scriptextern.cpp \
	scriptfuncs.cpp \
	scriptobj.cpp \
	scripttabs.cpp \
	scriptvals.cpp \
	scriptvals_lexer.cpp \
	scriptvals_parser.cpp \
	selection.cpp \
	seqdisp.cpp \
	stats.cpp \
	structure.cpp \
	template.cpp \
	terrain.cpp \
	text.cpp \
	texture.cpp \
	transporter.cpp \
	version.cpp \
	visibility.cpp \
	warcam.cpp \
	warzoneconfig.cpp \
	wavecast.cpp \
	wrappers.cpp



COMMONLIBS = \
	$(top_builddir)/lib/widget/libwidget.a \
	$(top_builddir)/lib/sequence/libsequence.a \
	$(top_builddir)/lib/sound/libsound.a \
	$(top_builddir)/lib/script/libscript.a \
	$(top_builddir)/lib/netplay/libnetplay.a \
	$(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/exceptionhandler/libexceptionhandler.a \
	$(top_builddir)/3rdparty/miniupnpc/libminiupnpc.a

if PORTABLE
warzone2100_portable_SOURCES = $(COMMONSOURCES) $(nodist_COMMONSOURCES)
warzone2100_portable_LIBS = $(COMMONLIBS)
else
warzone2100_SOURCES = $(COMMONSOURCES) $(nodist_COMMONSOURCES)
warzone2100_LIBS = $(COMMONLIBS)
endif

if BACKEND_QT
if PORTABLE
warzone2100_portable_LIBS += $(top_builddir)/lib/qtgame/libqtgame.a
else
warzone2100_LIBS += $(top_builddir)/lib/qtgame/libqtgame.a
endif
endif

if BACKEND_SDL
if PORTABLE
warzone2100_portable_LIBS += $(top_builddir)/lib/sdl/libsdl.a
else
warzone2100_LIBS += $(top_builddir)/lib/sdl/libsdl.a
endif
endif

# lib/framework (and dependencies)
if PORTABLE
warzone2100_portable_LIBS += \
	$(top_builddir)/lib/framework/libframework.a \
	$(top_builddir)/3rdparty/micro-ecc/libmicroecc.a \
	$(top_builddir)/3rdparty/sha2/libsha2.a
else
warzone2100_LIBS += \
	$(top_builddir)/lib/framework/libframework.a \
	$(top_builddir)/3rdparty/micro-ecc/libmicroecc.a \
	$(top_builddir)/3rdparty/sha2/libsha2.a
endif

# Here, we need to bring in the "plugins" for Qt5 and they MUST come before the other QT libs.
# FIXME: At a future date, we should check if they want static builds, but, since we only support those for now...
if MINGW32
SQT5_LIBS =  -L$(QT5_PLUGIN)/plugins/platforms -lQt5PlatformSupport -lqwindows
else
SQT5_LIBS =
endif

if PORTABLE
warzone2100_portable_LDADD = $(warzone2100_portable_LIBS) $(LTLIBINTL) $(SDL_LIBS) $(PHYSFS_LIBS) $(PNG_LIBS) $(VORBISFILE_LIBS) $(VORBIS_LIBS) \
	$(THEORA_LIBS) $(OPENAL_LIBS) $(FONT_LIBS) $(OPENGL_LIBS) $(SQT5_LIBS) $(QT5_LIBS) $(GLEW_LIBS) \
	$(X_LIBS) $(X_EXTRA_LIBS) $(LDFLAGS)
else
warzone2100_LDADD = $(warzone2100_LIBS) $(LTLIBINTL) $(SDL_LIBS) $(PHYSFS_LIBS) $(PNG_LIBS) $(VORBISFILE_LIBS) $(VORBIS_LIBS) \
	$(THEORA_LIBS) $(OPENAL_LIBS) $(FONT_LIBS) $(OPENGL_LIBS) $(SQT5_LIBS) $(QT5_LIBS) $(GLEW_LIBS) \
	$(X_LIBS) $(X_EXTRA_LIBS) $(LDFLAGS)
endif


if MINGW32
if PORTABLE
warzone2100_portable_LDADD += $(top_builddir)/win32/warzone2100_portable.o $(WIN32_LIBS)
else
warzone2100_LDADD += $(top_builddir)/win32/warzone2100.o  $(WIN32_LIBS)
endif
endif
