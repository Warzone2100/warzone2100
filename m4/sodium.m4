# Derived from: https://github.com/dovecot/core/blob/master/m4/want_sodium.m4
# License: https://github.com/dovecot/core/blob/master/COPYING.LGPL

AC_DEFUN([WZ_FIND_SODIUM], [
    PKG_CHECK_MODULES(LIBSODIUM, libsodium, [
      OLD_LIBS="$LIBS"
      LIBS="$LIBS $LIBSODIUM_LIBS"
      AC_CHECK_FUNC([crypto_pwhash_str_verify], [
        have_sodium=yes
        AC_DEFINE(HAVE_LIBSODIUM, [1], [Define if you have libsodium])
      ])
      LIBS="$OLD_LIBS"
    ], [have_sodium=no])
    AS_IF([test "$have_sodium" != "yes"] , [
      AC_ERROR([Can't build with libsodium: not found])
    ])
  AM_CONDITIONAL(BUILD_LIBSODIUM, test "$have_sodium" = "yes")
])dnl
