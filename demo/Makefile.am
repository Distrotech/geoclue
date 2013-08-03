noinst_PROGRAMS = where-am-i

where_am_i_SOURCES = where-am-i.c
where_am_i_CFLAGS = -I$(top_srcdir)/src 		\
                    -I$(top_builddir)/src 		\
		    $(GEOCLUE_CFLAGS)   		\
		    -DLOCALEDIR="\"$(datadir)/locale\""
where_am_i_LDADD = $(top_builddir)/src/libgeoclue-1.0.la \
		   $(GEOCLUE_LIBS)
