include $(top_srcdir)/Makefile.decl

libexec_PROGRAMS = geoclue

install-exec-hook:
	$(AM_V_GEN)$(SETCAP) cap_sys_ptrace+ep $(DESTDIR)$(libexecdir)/geoclue || \
	echo "Unable to setcap cap_sys_ptrace+ep $(libexecdir)/geoclue. " \
	     "Package systems may do this after installation."

SUBDIRS = agent geocode-glib

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
	gclue-enum-types.h	\
	gclue-enum-types.c	\
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
		$(builddir)/geocode-glib/libgeocode-glib.la

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(interface_DATA) \
	     test-data/fedora-geoip-results.json \
	     test-data/freegeoip-results.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geoclue-2.0.pc

# Enum types
GEOCLUE_ENUMS = \
	$(srcdir)/gclue-enums.h

gclue-enum-types.h: Makefile.am $(GEOCLUE_ENUMS) $(top_srcdir)/build-aux/gclue-enums-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
	    --fhead "#include \"gclue-enums.h\"\n#ifndef __GCLUE_ENUM_TYPES_H__\n#define __GCLUE_ENUM_TYPES_H__\n" \
	    --template $(top_srcdir)/build-aux/gclue-enums-template.h \
	    --ftail "#endif /* __GCLUE_ENUM_TYPES_H__ */\n" \
	    $(GEOCLUE_ENUMS) > $@

gclue-enum-types.c: Makefile.am $(top_srcdir)/build-aux/gclue-enums-template.c gclue-enum-types.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
	    --fhead "#include \"gclue-enum-types.h\"" \
	    --template $(top_srcdir)/build-aux/gclue-enums-template.c \
	    $(GEOCLUE_ENUMS) > $@

-include $(top_srcdir)/git.mk
