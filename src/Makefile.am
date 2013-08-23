include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

if BUILD_GEOIP_SERVER
SUBDIRS = geoip-server
else
SUBDIRS =
endif # BUILD_GEOIP_SERVER

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

CLEANFILES = $(BUILT_SOURCES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-2.0.pc

-include $(top_srcdir)/git.mk
