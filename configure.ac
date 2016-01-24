AC_INIT([sdlquake], [1.0.9])
AC_CONFIG_SRCDIR([WinQuake/README.SDL])
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AS

dnl set CFLAGS
case "$target" in
    i686-*-linux*)
        CFLAGS="$CFLAGS -mtune=atom -Ofast -funroll-loops -fomit-frame-pointer"
        ;;
    alpha*-*-linux*)
        CFLAGS="$CFLAGS -mcpu=ev4 -Wa,-mall"
        ;;
esac

dnl set MATHLIB and INETLIB
case "$target" in
    *-*-mingw32*)
        MATHLIB=""
        INETLIB="-lwsock32"
        ;;
    *-*-beos*)
        MATHLIB=""
        INETLIB=""
        ;;
    *)
        MATHLIB="-lm"
        INETLIB=""
        ;;
esac

AC_SUBST(MATHLIB)
AC_SUBST(INETLIB)

dnl find SDL
SDL_VERSION=1.0.1
AM_PATH_SDL($SDL_VERSION,
            :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS -DSDL"
LIBS="$LIBS $SDL_LIBS"

dnl set icon
case "$target" in
    *-*-mingw32*)
        cp ico_o ico.o
	ICONOBJ="ico.o"
        ;;
    *)
	ICONOBJ=""
        ;;
esac

AC_SUBST(ICONOBJ)

dnl --enable-asm
AC_ARG_ENABLE(asm,
[  --enable-asm            enable i686 assembly routines [default=no]],
                  , enable_asm=no)
if test x$enable_asm = xyes; then
    CFLAGS="$CFLAGS -DUSE_ASM"
    CCASFLAGS="$CCASFLAGS -DUSE_ASM"
fi

CFLAGS="$CFLAGS -DELF"
CCASFLAGS="$CCASFLAGS -DELF"

AC_CONFIG_FILES([Makefile WinQuake/Makefile])
AC_OUTPUT

