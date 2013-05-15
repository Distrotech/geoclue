servicedir       = $(datadir)/dbus-1/services
service_in_files = org.freedesktop.GeoClue2.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = $(service_in_files)
