sysconf_in_files = geoclue.conf.in
sysconf_DATA = $(sysconf_in_files:.conf.in=.conf)

# abs_top_srcdir does not resolve symlinks so we need this
$(sysconf_DATA): $(sysconf_in_files) Makefile
	@sed -e "s|\@abs_top_srcdir\@|`$(REALPATH) $(abs_top_srcdir)`|" $< > $@

servicedir       = $(datadir)/dbus-1/system-services
service_in_files = org.freedesktop.GeoClue2.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< | \
	sed -e "s|\@dbus_srv_user\@|$(dbus_srv_user)|" > $@

# DBus Service policy file
dbusservicedir = $(DBUS_SYS_DIR)
dbusservice_DATA = org.freedesktop.GeoClue2.conf \
		   org.freedesktop.GeoClue2.Agent.conf

systemdservice_in_files = geoclue.service.in

if HAVE_SYSTEMD
systemdservicedir       = $(systemdsystemunitdir)
systemdservice_DATA     = $(systemdservice_in_files:.service.in=.service)

$(systemdservice_DATA): $(systemdservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< | \
	sed -e "s|\@dbus_srv_user\@|$(dbus_srv_user)|" > $@
endif

DISTCLEANFILES = $(service_DATA)
EXTRA_DIST = $(service_in_files) \
             $(systemdservice_in_files) \
             $(dbusservice_DATA)

-include $(top_srcdir)/git.mk
