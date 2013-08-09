include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

SUBDIRS = geoip-server

dbus_built_sources = geoclue-interface.c geoclue-interface.h
geoclue-interface.c: geoclue-interface.h
geoclue-interface.h: Makefile.am geoclue-interface.xml
	gdbus-codegen						\
		--interface-prefix org.freedesktop.GeoClue2.	\
		--c-namespace GClue				\
		--generate-c-code geoclue-interface		\
		$(srcdir)/geoclue-interface.xml

BUILT_SOURCES = 		\
	$(dbus_built_sources)	\
	$(NULL)

geoclue_SOURCES =	 	 \
	$(BUILT_SOURCES) 	 \
	gclue-main.c	 	 \
	gclue-error.h 	 	 \
	gclue-error.c 	 	 \
	gclue-ipclient.h 	 \
	gclue-ipclient.c 	 \
	gclue-location-info.h 	 \
	gclue-location-info.c 	 \
	gclue-locator.h 	 \
	gclue-locator.c 	 \
	gclue-service-manager.h  \
	gclue-service-manager.c  \
	gclue-service-client.h   \
	gclue-service-client.c   \
	gclue-service-location.h \
	gclue-service-location.c \
	$(NULL)

geoclue_CFLAGS = $(GEOCLUE_CFLAGS) 		     \
		 -DLOCALEDIR="\"$(datadir)/locale\""
geoclue_LDADD = $(GEOCLUE_LIBS) $(LIBS)

lib_LTLIBRARIES = libgeoclue-1.0.la
libgeoclue_1_0_ladir = $(includedir)/geoclue-1.0

libgeoclue_1_0_la_HEADERS = geoclue.h geoclue-interface.h
libgeoclue_1_0_la_SOURCES = geoclue-interface.c
libgeoclue_1_0_la_CFLAGS = $(GEOCLUE_CFLAGS)
libgeoclue_1_0_la_LIBADD = $(GEOCLUE_LIBS)

CLEANFILES = $(BUILT_SOURCES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-2.0.pc

if HAVE_INTROSPECTION
girdir = $(datadir)/gir-1.0
gir_DATA = Geoclue-1.0.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = Geoclue-1.0.typelib

Geoclue-1.0.gir: $(libgeoclue_1_0_la_SOURCES) $(libgeoclue_1_0_la_HEADERS)
	$(INTROSPECTION_SCANNER) -v		\
	    --warn-all				\
	    --namespace Geoclue			\
	    --identifier-prefix=GClue 		\
	    --symbol-prefix=gclue		\
	    --nsversion=1.0			\
	    --include=Gio-2.0			\
	    --library=geoclue-1.0		\
	    --output $@				\
	    --pkg=glib-2.0			\
	    --pkg=gobject-2.0			\
	    --pkg=gio-2.0			\
	    --pkg-export=geoclue-1.0		\
	    --libtool=$(top_builddir)/libtool	\
	    --c-include='geoclue.h'		\
	    -I$(top_srcdir)/src			\
	    -I$(top_builddir)/src		\
	    $^					\
	    $(NULL)

Geoclue-1.0.gir: libgeoclue-1.0.la $(INTROSPECTION_SCANNER) Makefile.am
Geoclue-1.0.typelib: Geoclue-1.0.gir $(INTROSPECTION_COMPILER)
	$(INTROSPECTION_COMPILER) $< -o $@

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
EXTRA_DIST = geoclue-interface.xml			 \
	     test-data/freegeoip-results.json

endif # HAVE_INTROSPECTION

-include $(top_srcdir)/git.mk
