#  * Version: $Id: Makefile.am,v 1.7 2015/10/20 19:41:50 nroche Exp $
#  * Project: MediaTex
#  * Module:  scripts files
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2014 2015 Nicolas Roche
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

init_ddir = $(sysconfdir)/init.d
scriptsdir = $(datarootdir)@mdtx_mediatexdir@/scripts
scriptslibdir = $(scriptsdir)/lib

dist_init_d_SCRIPTS = \
	mediatexd

dist_scripts_SCRIPTS = \
	init.sh remove.sh purge.sh \
	new.sh free.sh clean.sh \
	update.sh commit.sh \
	addUser.sh delUser.sh \
	bind.sh unbind.sh \
	mount.sh umount.sh \
	deliver.sh audit.sh \
	cron_hourly.sh cron_daily.sh cron_monthly.sh

dist_scriptslib_SCRIPTS = \
	lib/log.sh \
	lib/include.sh \
	lib/users.sh \
	lib/htdocs.sh \
	lib/ssh.sh \
	lib/cvs.sh \
	lib/jail.sh

EXTRA_DIST = \
	lib/include.src \
	mediatexd.src \
	cron_hourly.src \
	cron_daily.src \
	cron_monthly.src
CLEANFILES = \
	lib/include.sh \
	mediatexd \
	cron_hourly.sh \
	cron_daily.sh \
	cron_monthly.sh

lib/include.sh: lib/include.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

mediatexd: mediatexd.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

cron_hourly.sh: cron_hourly.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

cron_daily.sh: cron_daily.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@

cron_monthly.sh: cron_monthly.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@
