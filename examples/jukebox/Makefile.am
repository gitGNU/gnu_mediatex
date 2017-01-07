#  * Project: Jukebox
#  * Module: root
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2017 Nicolas Roche
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

dist_bin_SCRIPTS = bin/jukebox.sh

jukeboxdir = $(sysconfdir)/mediatex
dist_jukebox_DATA = etc/jukebox.conf

rsyslogdir = $(sysconfdir)/rsyslog.d
dist_rsyslog_DATA = etc/jukebox_rsyslog.conf

EXTRA_DIST = \
	bin/jukebox.src

CLEANFILES = \
	bin/jukebox.sh	

#po
SUBDIRS = . doc check man

bin/jukebox.sh:	bin/jukebox.src
	sed $< \
		-e "s!{PACKAGE_BUGREPORT}!@PACKAGE_BUGREPORT@!g" \
		-e "s!{PACKAGE_NAME}!@PACKAGE_NAME@!g" \
		-e "s!{PACKAGE_STRING}!@PACKAGE_STRING@!g" \
		-e "s!{SYSCONFDIR}!$(sysconfdir)!g" \
		-e "s!{LOCALSTATEDIR}!$(localstatedir)!g" \
		> $@
	chmod +x $@

# '|| true' needed for 'distcheck'
install-data-hook:
	cd $(DESTDIR)$(rsyslogdir); \
	mv jukebox_rsyslog.conf jukebox.conf || true

uninstall-hook:
	rm -f $(DESTDIR)$(rsyslogdir)/jukebox.conf
