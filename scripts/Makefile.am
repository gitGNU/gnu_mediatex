#  * Version: $Id: Makefile.am,v 1.3 2015/06/03 14:03:22 nroche Exp $
#  * Project: MediaTex
#  * Module:  scripts files (called by C code)
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

# Build recursively 
SUBDIRS = . lib

init_ddir = $(sysconfdir)/init.d
dist_init_d_SCRIPTS = mediatexd

scriptsdir = $(datarootdir)@mdtx_mediatexdir@/scripts
dist_scripts_SCRIPTS = \
	init.sh remove.sh purge.sh \
	new.sh free.sh clean.sh \
	update.sh commit.sh \
	addUser.sh delUser.sh \
	bind.sh unbind.sh \
	mount.sh umount.sh \
	deliver.sh \
	cron_hourly.sh cron_daily.sh

EXTRA_DIST = mediatexd.src cron_hourly.src cron_daily.src
CLEANFILES = mediatexd cron_hourly.sh cron_daily.sh

do_subst = sed \
 -e "s!CONF_MEDIATEX!$(mdtx_mediatexdir)!" \
 -e "s!CONF_BINDIR!$(bindir)!" \
 -e "s!CONF_LOCALSTATEDIR!$(localstatedir)!" \
 -e "s!CONF_SYSCONFDIR!$(sysconfdir)!g"

mediatexd: mediatexd.src $(top_srcdir)/mediatex-config.h
	$(do_subst) < $< > $@

cron_hourly.sh: cron_hourly.src $(top_srcdir)/mediatex-config.h
	$(do_subst) < $< > $@

cron_daily.sh: cron_daily.src $(top_srcdir)/mediatex-config.h
	$(do_subst) < $< > $@

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
