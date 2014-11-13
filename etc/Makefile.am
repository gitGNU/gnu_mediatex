#  * Version: $Id: Makefile.am,v 1.2 2014/11/13 16:36:08 nroche Exp $
#  * Project: MediaTex
#  * Module:  configuration files
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2014  Nicolas Roche
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

apachedir = $(sysconfdir)/apache2/conf.d
dist_apache_DATA = mediatex.conf

rsyslogdir = $(sysconfdir)/rsyslog.d
dist_rsyslog_DATA = mediatex_log.conf

logrotatedir = $(sysconfdir)/logrotate.d
dist_logrotate_DATA = mediatex_rotate.conf

ldsoconfdir = $(sysconfdir)/ld.so.conf.d
dist_ldsoconf_DATA = mediatex_ld.conf

completiondir = $(sysconfdir)/bash_completion.d
dist_completion_DATA = mediatex_comp

EXTRA_DIST = apache.src rsyslog.src logrotate.src ld.src
CLEANFILES = mediatex.conf mediatex_log.conf mediatex_ld.conf

mediatex.conf: apache.src $(top_srcdir)/config.h
	sed $< \
		-e "s!/MEDIATEX!$(mdtx_mediatexdir)!" \
		-e "s!CACHEDIR!$(localstatedir)/cache$(mdtx_mediatexdir)!" \
		> $@

mediatex_log.conf: rsyslog.src $(top_srcdir)/config.h
	sed $< \
		-e "s!/MEDIATEX!$(mdtx_mediatexdir)!" \
		> $@

mediatex_rotate.conf: logrotate.src $(top_srcdir)/config.h
	sed $< \
		-e "s!/MEDIATEX!$(mdtx_mediatexdir)!" \
		> $@

mediatex_ld.conf: ld.src $(top_srcdir)/config.h
	sed $< \
		-e "s!LIBDIR!$(libdir)!" \
		-e "s!/MEDIATEX!$(mdtx_mediatexdir)!" \
		> $@

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
