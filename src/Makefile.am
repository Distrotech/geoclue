include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

SUBDIRS = public-api agent geocode-glib

if BUILD_GEOIP_SERVER
SUBDIRS += geoip-server
endif # BUILD_GEOIP_SERVER

interfacedir = $(datadir)/geoclue-2.0
interface_DATA = geoclue-interface.xml

dbus_built_sources = geoclue-interface.c geoclue-interface.h
geoclue-interface.c: geoclue-interface.h
geoclue-interface.h: Makefile.am $(interface_DATA)
	$(AM_V_GEN)$(GDBUS_CODEGEN) \
		--interface-prefix org.freedesktop.GeoClue2.	\
		--c-namespace GClue				\
		--generate-c-code geoclue-interface		\
    		--generate-docbook=docs				\
		$(srcdir)/$(interface_DATA)

BUILT_SOURCES = 		\
	$(dbus_built_sources)	\
	$(NULL)

noinst_LTLIBRARIES = libgeoclue.la

AM_CPPFLAGS = $(GEOCLUE_CFLAGS) 		     	  \
	      $(WARN_CFLAGS)			     	  \
	      -DLOCALEDIR="\"$(datadir)/locale\"" 	  \
	      -DG_LOG_DOMAIN=\""Geoclue"\"	     	  \
	      -DABS_TOP_SRCDIR=\""$(abs_top_srcdir)"\" \
	      -DSYSCONFDIR=\""$(sysconfdir)"\"

libgeoclue_la_SOURCES =	 	 \
	$(BUILT_SOURCES) 	 \
	gclue-client-info.h 	 \
	gclue-client-info.c 	 \
	gclue-config.h 	 	 \
	gclue-config.c 	 	 \
	gclue-error.h 	 	 \
	gclue-error.c 	 	 \
	gclue-ipclient.h 	 \
	gclue-ipclient.c 	 \
	gclue-location-source.h	 \
	gclue-location-source.c	 \
	gclue-locator.h 	 \
	gclue-locator.c 	 \
	gclue-modem-source.c	 \
	gclue-modem-source.h	 \
	gclue-modem-gps.c	 \
	gclue-modem-gps.h	 \
	gclue-service-manager.h  \
	gclue-service-manager.c  \
	gclue-service-client.h   \
	gclue-service-client.c   \
	gclue-service-location.h \
	gclue-service-location.c \
	gclue-wifi.h 	 	 \
	gclue-wifi.c 	 	 \
	gclue-web-source.c	 \
	gclue-web-source.h	 \
	gclue-3g.c	 	 \
	gclue-3g.h	 	 \
	gclue-enums.h		 \
	$(NULL)
libgeoclue_la_LIBADD = $(GEOCLUE_LIBS) $(LIBS)

geoclue_SOURCES =	 	  \
	gclue-main.c	 	  \
	$(NULL)
geoclue_LDADD = $(GEOCLUE_LIBS) 	  		    \
		$(LIBS) 		  		    \
		$(builddir)/libgeoclue.la		    \
		$(builddir)/geocode-glib/libgeocode-glib.la \
		$(builddir)/public-api/libgeoclue-public-api.la

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(interface_DATA) \
	     test-data/fedora-geoip-results.json \
	     test-data/freegeoip-results.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-2.0.pc

-include $(top_srcdir)/git.mk
