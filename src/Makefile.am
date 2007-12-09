%.tab.h %.tab.c: %.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -o$@ $<

%.lex.c: %.l
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -o$@ $<

AM_LFLAGS = $(FLEX_FLAGS)
AM_YFLAGS = -d
BUILT_SOURCES = level_lexer.lex.c scriptvals_lexer.lex.c scriptvals_parser.tab.c scriptvals_parser.tab.h
CLEANFILES = level_lexer.lex.c scriptvals_lexer.lex.c scriptvals_parser.tab.c scriptvals_parser.tab.h

bin_PROGRAMS = warzone2100
noinst_HEADERS = action.h advvis.h ai.h aiexperience.h anim_id.h \
	astar.h atmos.h basedef.h bridge.h bucket3d.h \
	projectiledef.h cdspan.h cheat.h clparse.h cluster.h cmddroid.h \
	cmddroiddef.h combat.h component.h configuration.h console.h data.h \
	deliverance.h design.h difficulty.h display.h display3d.h display3ddef.h \
	displaydef.h drive.h droid.h droiddef.h e3demo.h edit3d.h effects.h \
	environ.h feature.h featuredef.h formation.h formationdef.h fpath.h \
	frend.h frontend.h function.h functiondef.h game.h gateway.h \
	gatewaydef.h gatewayroute.h geometry.h group.h hci.h ingameop.h init.h intdisplay.h \
	intelmap.h intfac.h intimage.h intorder.h keybind.h keyedit.h keymap.h levelint.h \
	levels.h lighting.h loadsave.h loop.h map.h mapdisplay.h mapgrid.h mechanics.h \
	message.h messagedef.h miscimd.h mission.h missiondef.h move.h movedef.h \
	multigifts.h multiint.h multijoin.h multilimit.h multimenu.h multiplay.h multirecv.h \
	multistat.h objectdef.h objects.h objmem.h oprint.h optimisepath.h order.h \
	orderdef.h power.h projectile.h radar.h raycast.h research.h \
	researchdef.h resource.h scores.h scriptai.h scriptcb.h scriptextern.h scriptfuncs.h \
	scriptobj.h scripttabs.h scriptvals.h selection.h seqdisp.h stats.h statsdef.h \
	structure.h structuredef.h target.h text.h texture.h transporter.h visibility.h \
	warcam.h warzoneconfig.h weapondef.h main.h wrappers.h

warzone2100_SOURCES = scriptvals_parser.tab.c scriptvals_lexer.lex.c \
	level_lexer.lex.c ai.c aiexperience.c astar.c action.c advvis.c atmos.c bridge.c \
	cdspan.c cheat.c cluster.c cmddroid.c combat.c component.c console.c \
	data.c design.c difficulty.c display.c droid.c e3demo.c \
	edit3d.c effects.c environ.c fpath.c feature.c formation.c frontend.c \
	gateway.c gatewayroute.c gatewaysup.c geometry.c group.c hci.c init.c \
	intdisplay.c intimage.c intorder.c intelmap.c keybind.c keymap.c levels.c lighting.c \
	loop.c main.c map.c mapdisplay.c mapgrid.c mechanics.c message.c miscimd.c \
	move.c multiint.c multimenu.c multiopt.c multisync.c multibot.c multistat.c \
	objmem.c objects.c optimisepath.c order.c radar.c \
	raycast.c research.c scores.c scriptai.c scriptcb.c scriptextern.c scriptfuncs.c \
	scriptobj.c scripttabs.c scriptvals.c selection.c stats.c text.c texture.c \
	transporter.c visibility.c warcam.c wrappers.c aud.c \
	bucket3d.c clparse.c configuration.c display3d.c drive.c function.c game.c \
	ingameop.c keyedit.c loadsave.c mission.c multigifts.c multijoin.c multilimit.c \
	multiplay.c multistruct.c oprint.c power.c projectile.c seqdisp.c structure.c \
	target.c version.c warzoneconfig.c

warzone2100_LDADD = \
	$(top_builddir)/lib/widget/libwidget.a \
	$(top_builddir)/lib/sequence/libsequence.a \
	$(top_builddir)/lib/sound/libsound.a \
	$(top_builddir)/lib/script/libscript.a \
	$(top_builddir)/lib/netplay/libnetplay.a \
	$(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/ivis_common/libivis_common.a \
	$(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a

if MINGW32
warzone2100_LDADD += $(WIN32_PRE_LIBS)
endif

warzone2100_LDADD += $(SDL_LIBS) $(SDL_NET_LIBS) $(PHYSFS_LIBS) $(PNG_LIBS) $(OGGVORBIS_LIBS) $(OPENAL_LIBS) $(OPENGLC_LIBS) $(OPENGL_LIBS) $(POPT_LIBS)

if MINGW32
warzone2100_LDADD += $(top_builddir)/win32/warzone2100.o $(WIN32_LIBS)
endif
