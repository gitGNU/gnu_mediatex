#  * Version: $Id: Makefile.am,v 1.1 2014/10/13 19:38:18 nroche Exp $
#  * Project: MediaTex
#  * Module:  inital configuration files (and testing/exemple purpose)
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

examplesdir = $(datarootdir)@mdtx_mediatexdir@/examples
dist_examples_DATA = \
	mediatex_cron mediatex.conf supports.txt \
	catalog00.txt extract00.txt servers.txt \
	index.htaccess cache.htaccess score.htaccess cgi.htaccess \
	apache-mdtx.conf htgroup viewvc.conf header.ezt \
	mediatex.css floppy-icon.png home.htaccess \
	logo.png logoP1.iso logoP2.iso logoP1.cat logoP2.cat\
	logo.tgz logo.tar.bz2 logo.afio \
	logo.tar logo.cpio.gz logo.cpio logo.cpio.bz2 \
	logo.zip logo.part1.rar logo.part2.rar

# dist_check_DATA = // need to modify servers modules

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
