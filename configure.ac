AC_PREREQ([2.63])

AC_INIT([geoclue],
        [1.99.0],
        [http://bugs.freedesktop.org/enter_bug.cgi?product=GeoClue],
        [geoclue],
        [http://www.freedesktop.org/wiki/Software/GeoClue])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 dist-xz no-dist-gzip tar-ustar foreign -Wall -Werror])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AM_PROG_AR
AC_CHECK_TOOL([DESKTOP_FILE_VALIDATE], [desktop-file-validate], [/bin/true])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Pkg-config
PKG_PROG_PKG_CONFIG([0.22])

# i18 support
IT_PROG_INTLTOOL(0.40.0)

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The name of the gettext domain])

# GObject-introspection support
GOBJECT_INTROSPECTION_REQUIRE([0.9.6])

# Mallard help
YELP_HELP_INIT

# GSettings support
GLIB_GSETTINGS

GLIB_MIN_VERSION=2.32.0
GEOCODE_MIN_VERSION=0.99.1

PKG_CHECK_MODULES(GEOCLUE, [
  glib-2.0 >= $GLIB_MIN_VERSION
  gio-2.0 >= $GLIB_MIN_VERSION
  gio-unix-2.0 >= $GLIB_MIN_VERSION
  geocode-glib-1.0 >= $GEOCODE_MIN_VERSION
])

dnl Strict compiler
AC_ARG_ENABLE([strict-cc],
  AS_HELP_STRING([--enable-strict-cc],[Enable strict C compiler]))

AS_IF([test "x$enable_strict_cc" = "xyes"], [CFLAGS="$CFLAGS -Wall -Werror"],
      [CFLAGS="$CFLAGS -w"])

dnl Debugging
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[enable debugging]),,
        enable_debug=no)
if test "x$enable_debug" = "xyes"; then
        CFLAGS="$CFLAGS -O0 -ggdb3"
fi

GOBJECT_INTROSPECTION_CHECK([0.6.2])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/geoclue-2.0.pc
  po/Makefile.in
  data/Makefile
  demo/Makefile
])

AC_OUTPUT

dnl ==========================================================================
AC_MSG_NOTICE([

        GeoClue $VERSION
        =================

        prefix:                   ${prefix}
        c compiler:               ${CC} ${CFLAGS}
])
