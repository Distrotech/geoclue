noinst_PROGRAMS = where-am-i

where_am_i_SOURCES = where-am-i.c
where_am_i_CFLAGS = -I$(top_srcdir)/src 		\
		    $(GEOCLUE_CFLAGS)   		\
		    -DLOCALEDIR="\"$(datadir)/locale\""
where_am_i_LDADD = -L$(top_srcdir)/src \
		   -lgeoclue-1.0       \
		   $(GEOCLUE_LIBS)
