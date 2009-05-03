#! /usr/bin/env python
# encoding: utf-8
# Dennis Schridde, 2006 (devurandom)

# Warzone 2100

# the following two variables are used by the target "waf dist"
VERSION='TRUNK'
APPNAME='Warzone 2100'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = '_build_'

# make sure waf has the version we want
import Utils
Utils.waf_version(mini="1.0", maxi="9.9.9")


# For DEFAULT_DATADIR and debug variant
import Params


def set_options(opt):
	#add option for additional/external includes and library dirs for MinGW on Windows
	opt.tool_options('compiler_cc')
	opt.add_option('--debug',
		action='store_true',
		default=False,
		help='Build debug variant [Default: False]',
		dest='debug')


def configure(conf):
	# Check for C-Compiler, the Lexer/Parser tools and get the checks module for checkEndian
	conf.check_tool('compiler_cc batched_cc flex bison checks')

	# Big or little endian?
	conf.checkEndian()

	# Check for all required libs
	if not conf.check_pkg2('sdl', '1.2', 0):
		if not conf.check_cfg2('sdl', 0):
			conf.check_header('SDL/SDL.h')
			conf.check_library('SDL')
			conf.check_library('SDLmain')

	if not conf.check_pkg2('openal', '0', 0):
		conf.check_header2('AL/al.h')
		conf.check_library2('openal')

	if not conf.check_pkg2('libpng', '1.2', 0, 'PNG'):
		conf.check_header2('png.h')
		conf.check_library2('png')

	if not conf.check_pkg2('ogg', '1.0', 0):
		conf.check_header2('ogg/ogg.h')
		conf.check_library2('ogg')

	if not conf.check_pkg2('vorbisfile', '1.0', 0):
		conf.check_header2('vorbis/vorbisfile.h')
		conf.check_library2('vorbisfile')

	conf.check_header2('GL/gl.h')
	conf.check_library2('GL')

	conf.check_header2('GL/glu.h')
	conf.check_library2('GLU')

	conf.check_header2('physfs.h')
	conf.check_library2('physfs')

	# Common defines
	conf.add_define('VERSION', VERSION)
	conf.add_define('DEFAULT_DATADIR', Params.g_options.prefix + 'warzone2100')
	conf.add_define('YY_STATIC', 1)
	conf.add_define('LOCALEDIR', '')
	conf.add_define('PACKAGE', 'warzone2100')
	conf.env['CCFLAGS'] += ['-pipe', '-combine']

	# Split off debug variant before adding variant specific defines
	conf.set_env_name('debug', conf.env.deepcopy())

	# Configure non debug variant
	conf.env['CCFLAGS'] += ['-DNDEBUG', '-g', '-O2']
	conf.add_define('NDEBUG', 1)
	conf.write_config_header()

	# Configure debug variant
	conf.setenv('debug')
	conf.env.set_variant('debug')
	conf.env['CCFLAGS'] += ['-DDEBUG', '-g', '-O0', '-Wall', '-Wextra']
	conf.add_define('DEBUG', 1)
	conf.write_config_header()


def build(bld):
	obj = bld.create_obj('cc', 'program')
	obj.find_sources_in_dirs('lib/framework lib/gamelib lib/netplay lib/ivis_common lib/ivis_opengl lib/script lib/sequence lib/sound lib/widget src')
	obj.uselib='PNG OGG VORBISFILE GLU GL OPENAL PHYSFS SDL SDLMAIN'
	obj.includes='lib/framework lib/gamelib lib/script src'
	obj.defines='HAVE_CONFIG_H'
	obj.target='warzone2100'

	# Use debug environment when --enable-debug is given
	if Params.g_options.debug:
		obj.env = bld.env_of_name('debug')
