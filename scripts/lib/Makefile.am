#  * Version: $Id: Makefile.am,v 1.1 2014/10/13 19:38:37 nroche Exp $
#  * Project: MediaTex
#  * Module:  scripts libraries (called by scripts)
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

scriptsdir = $(datarootdir)@mdtx_mediatexdir@/scripts/lib
dist_scripts_SCRIPTS = \
	include.sh \
	unitTest.sh \
	log.sh \
	users.sh \
	ssh.sh \
	cvs.sh \
	jail.sh \
	htdocs.sh

EXTRA_DIST = include.src 
CLEANFILES = include.sh

TESTS = \
	unitTest.sh \
	log.sh \
	include.sh \
	users.sh \
	ssh.sh \
	cvs.sh \
	jail.sh \
	htdocs.sh

dist_check_SCRIPTS = \
	include.src 

dist_check_DATA = \
	unitTest.exp \
	log.exp \
	include.exp \
	users.exp \
	ssh.exp \
	cvs.exp \
	jail.exp \
	htdocs.exp

do_subst = sed \
 -e "s!CONF_MEDIATEXDIR!$(mdtx_mediatexdir)!" \
 -e "s!CONF_BINDIR!$(bindir)!" \
 -e "s!CONF_DATAROOTDIR!$(datarootdir)!" \
 -e "s!CONF_EXEC_PREFIX!$(exec_prefix)!" \
 -e "s!CONF_LIBDIR!$(libdir)!" \
 -e "s!CONF_LOCALSTATEDIR!$(localstatedir)!" \
 -e "s!CONF_PREFIX!$(prefix)!" \
 -e "s!CONF_SYSCONFDIR!$(sysconfdir)!"

include.sh: include.src $(top_srcdir)/config.h
	$(do_subst) < $< > $@
	chmod +x $@

clean-local:
	-rm -f *.out

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
