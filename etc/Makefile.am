#  * Project: MediaTex
#  * Module:  configuration files
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2014 2015 2016 2017 Nicolas Roche
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

include $(top_srcdir)/Make-rules

ldsoconfdir = $(sysconfdir)/ld.so.conf.d
dist_ldsoconf_DATA = mediatex_ld.conf

apachedir = $(sysconfdir)/apache2/conf-available
dist_apache_DATA = mediatex_apache.conf

rsyslogdir = $(sysconfdir)/rsyslog.d
dist_rsyslog_DATA = mediatex_rsyslog.conf

logrotatedir = $(sysconfdir)/logrotate.d
dist_logrotate_DATA = mediatex_logrotate.conf

completiondir = $(sysconfdir)/bash_completion.d
dist_completion_DATA = mediatex_bash_completion

EXTRA_DIST = \
	ld.src \
	apache.src \
	rsyslog.src \
	logrotate.src
CLEANFILES = \
	mediatex_ld.conf \
	mediatex_apache.conf \
	mediatex_rsyslog.conf \
	mediatex_logrotate.conf

mediatex_ld.conf: ld.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_apache.conf: apache.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_rsyslog.conf: rsyslog.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_logrotate.conf: logrotate.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

# '|| true' needed for 'distcheck'
install-data-hook:
	cd $(DESTDIR)$(ldsoconfdir); \
	mv mediatex_ld.conf mediatex.conf || true
	cd $(DESTDIR)$(apachedir); \
	mv mediatex_apache.conf mediatex.conf || true
	cd $(DESTDIR)$(rsyslogdir); \
	mv mediatex_rsyslog.conf mediatex.conf || true
	cd $(DESTDIR)$(logrotatedir); \
	mv mediatex_logrotate.conf mediatex || true
	cd $(DESTDIR)$(completiondir); \
	mv mediatex_bash_completion mediatex || true

uninstall-hook:
	rm -f $(DESTDIR)$(ldsoconfdir)/mediatex.conf
	rm -f $(DESTDIR)$(apachedir)/mediatex.conf
	rm -f $(DESTDIR)$(rsyslogdir)/mediatex.conf
	rm -f $(DESTDIR)$(logrotatedir)/mediatex
	rm -f $(DESTDIR)$(completiondir)/mediatex
