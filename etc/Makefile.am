#  * Project: MediaTex
#  * Module:  configuration files
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2014 2015 2016 Nicolas Roche
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

apachedir = $(sysconfdir)/apache2/conf-available/
dist_apache_DATA = mediatex.conf

rsyslogdir = $(sysconfdir)/rsyslog.d
dist_rsyslog_DATA = mediatex_log.conf

logrotatedir = $(sysconfdir)/logrotate.d
dist_logrotate_DATA = mediatex_logrotate.conf

logrotateApachedir = $(logrotatedir)/httpd-prerotate
dist_logrotateApache_SCRIPTS = mediatex_logrotate

ldsoconfdir = $(sysconfdir)/ld.so.conf.d
dist_ldsoconf_DATA = mediatex_ld.conf

completiondir = $(sysconfdir)/bash_completion.d
dist_completion_DATA = mediatex_comp

EXTRA_DIST = \
	apache.src \
	rsyslog.src \
	logrotate.src \
	logrotate_apache.src \
	ld.src
CLEANFILES = \
	mediatex.conf \
	mediatex_log.conf \
	mediatex_logrotate.conf \
	mediatex_logrotate \
	mediatex_ld.conf

mediatex.conf: apache.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_log.conf: rsyslog.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_logrotate.conf: logrotate.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_logrotate: logrotate_apache.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatex_ld.conf: ld.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

