dnl SDL_image stuff
AC_DEFUN([AC_PATH_SDLNET],[
dnl
dnl Set SDLNETINC and SDLNETLIBS with needed cflags, libs to use SDL_net
dnl

dnl init
SDLNETINC=""
SDLNETLIB=""
SDLNETDIR=""

for dir in /usr/local /usr/X11R6 /usr; do
	if test -f "$dir/include/SDL/SDL_net.h"; then
		SDLNETINC="-I$dir/include/SDL"
		SDLNETLIB="-L$dir/lib"
		SDLNETDIR="$dir"
		break
	fi
done

_CPPFLAGS="$CPPFLAGS"
_LIBS="$LIBS"
CPPFLAGS="$SDL_CFLAGS $SDLNETINC"
LIBS="$SDL_LIBS $SDLNETLIB -lSDL_net"
AC_TRY_COMPILE([
	 #include "SDL.h"
	 #include "SDL/SDL_net.h"
	 int main(int argc, char **argv)
	 {
		 SDLNet_Init();
		 return 0
		/*
	],[
	 */
	], [SDLNETLIB="$SDLNETLIB -lSDL_net" SDLNETDIR="path"], [SDLNETDIR=""])
LIBS="$_LIBS"
CPPFLAGS="$_CPPFLAGS"

AC_MSG_CHECKING(for presence of SDL_net)

if test ! -z "$SDLNETDIR"; then
	AC_MSG_RESULT(Found SDL_net in $SDLNETDIR)
	$1
else
	AC_MSG_RESULT(Could not find SDL_net)
	$2
fi
])
