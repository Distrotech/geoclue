libexec_PROGRAMS = geoclue

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
	gclue-service-manager.h  \
	gclue-service-manager.c  \
	gclue-service-client.h   \
	gclue-service-client.c   \
	gclue-service-location.h \
	gclue-service-location.c \
	$(NULL)

geoclue_CFLAGS = $(GEOCLUE_CFLAGS) 		     \
		 -DLOCALEDIR="\"$(datadir)/locale\""
geoclue_LDADD = $(GEOCLUE_LIBS)

lib_LTLIBRARIES = libgeoclue-1.0.la
libgeoclue_1_0_ladir = $(includedir)/geoclue-1.0

libgeoclue_1_0_la_HEADERS = geoclue-interface.h
libgeoclue_1_0_la_SOURCES = geoclue-interface.c
libgeoclue_1_0_la_CFLAGS = $(GEOCLUE_CFLAGS)
libgeoclue_1_0_la_LIBADD = $(GEOCLUE_LIBS)

CLEANFILES = $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
