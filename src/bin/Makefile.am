#  * Version: $Id: Makefile.am,v 1.3 2015/06/03 14:03:27 nroche Exp $
#  * Project: MediaTex
#  * Module:  binaries
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

libmediatexdir = $(libdir)
libmediatex_LTLIBRARIES = libmediatex.la
libmediatex_ladir = $(includedir)

libmediatex_la_HEADERS = 
libmediatex_la_SOURCES = \
	$(misc_sources) \
	$(memory_sources) \
	$(parser_sources) \
	$(common_sources) \
	$(client_sources)

bin_PROGRAMS = \
	mediatex \
	mediatexd

cgidir = $(datarootdir)@mdtx_mediatexdir@/cgi-dir
cgi_PROGRAMS = cgi

mediatex_SOURCE = mediatex.c
mediatex_LDADD = libmediatex.la 
mediatex_LDFLAGS = @LIBINTL@ -pthread -lm -lavl -lcrypto

mediatexd_SOURCE = mediatexd.c
mediatexd_LDADD = \
	libmediatex.la \
	../server/libserver.a
mediatexd_LDFLAGS = -pthread -lm -lavl -lcrypto

cgi_SOURCE = cgi.c
cgi_LDADD = libmediatex.la
cgi_LDFLAGS = -pthread -lm -lavl -lcrypto

cgi.c: ../common/cgi.c
	cp $< .

#clean-local:
#	-rm -f $.so

# stupid test bellow, but needed by distcheck
if SETUID_INSTALL
install-exec-hook:
	/bin/chmod 4755 $(DESTDIR)$(bindir)/mediatex
	/bin/chmod 0755 $(DESTDIR)$(bindir)/mediatexd
endif

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent
