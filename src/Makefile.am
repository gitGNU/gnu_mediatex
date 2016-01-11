#  * Version: $Id: Makefile.am,v 1.11 2015/10/20 16:44:22 nroche Exp $
#  * Project: MediaTex
#  * Module: sources
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

AM_CFLAGS = "-Wall"

misc_headers = \
	misc/getcgivars.h \
	misc/log.h \
	misc/command.h \
	misc/setuid.h \
	misc/signals.h \
	misc/device.h \
	misc/progbar.h \
	misc/md5sum.h \
	misc/cypher.h \
	misc/address.h \
	misc/tcp.h \
	misc/shm.h \
	misc/perm.h \
	misc/keys.h \
	misc/locks.h \
	misc/html.h

memory_headers = \
	memory/strdsm.h \
	memory/ardsm.h \
	memory/cvsPrint.h \
	memory/archive.h \
	memory/confTree.h \
	memory/supportTree.h \
	memory/catalogTree.h \
	memory/extractTree.h \
	memory/serverTree.h \
	memory/recordTree.h \
	memory/cacheTree.h

parser_headers = \
	parser/supportFile.h \
	parser/supportFile.tab.h \
	parser/confFile.h \
	parser/confFile.tab.h \
	parser/serverFile.h \
	parser/serverFile.tab.h \
	parser/extractFile.h \
	parser/extractFile.tab.h \
	parser/catalogFile.h \
	parser/catalogFile.tab.h \
	parser/recordFile.h \
	parser/recordFile.tab.h

common_headers = \
	common/register.h \
	common/connect.h \
	common/ssh.h \
	common/upgrade.h \
	common/openClose.h \
	common/extractScore.h

client_headers = \
	client/mediatex-client.h \
	parser/shellQuery.h \
	parser/shellQuery.tab.h \
	client/serv.h \
	client/conf.h \
	client/supp.h \
	client/motd.h \
	client/commonHtml.h \
	client/catalogHtml.h \
	client/extractHtml.h \
	client/misc.h \
	client/upload.h

server_headers = \
	server/mediatex-server.h \
	server/cache.h \
	server/deliver.h \
	server/extract.h \
	server/upload.h \
	server/cgiSrv.h \
	server/have.h \
	server/notify.h \
	server/threads.h

mediatex_sources = \
	misc/getcgivars.c \
	misc/log.c \
	misc/alloc.c \
	misc/command.c \
	misc/setuid.c \
	misc/signals.c \
	misc/device.c \
	misc/progbar.c \
	misc/md5sum.c \
	misc/cypher.c \
	misc/address.c \
	misc/tcp.c \
	misc/shm.c \
	misc/perm.c \
	misc/keys.c \
	misc/locks.c \
	misc/html.c \
	memory/strdsm.c \
	memory/ardsm.c \
	memory/cvsPrint.c \
	memory/archive.c \
	memory/confTree.c \
	memory/supportTree.c \
	memory/catalogTree.c \
	memory/extractTree.c \
	memory/serverTree.c \
	memory/recordTree.c \
	memory/cacheTree.c \
	parser/supportFile.c \
	parser/supportFile.tab.c \
	parser/confFile.c \
	parser/confFile.tab.c \
	parser/serverFile.c \
	parser/serverFile.tab.c \
	parser/extractFile.c \
	parser/extractFile.tab.c \
	parser/catalogFile.c \
	parser/catalogFile.tab.c \
	parser/recordFile.c \
	parser/recordFile.tab.c \
	common/register.c \
	common/connect.c \
	common/ssh.c \
	common/upgrade.c \
	common/openClose.c \
	common/extractScore.c \
	client/commonHtml.c \
	client/catalogHtml.c \
	client/extractHtml.c

client_sources = \
	parser/shellQuery.c \
	parser/shellQuery.tab.c \
	client/serv.c \
	client/conf.c \
	client/supp.c \
	client/motd.c \
	client/misc.c \
	client/upload.c

server_sources = \
	server/cache.c \
	server/deliver.c \
	server/extract.c \
	server/upload.c \
	server/cgiSrv.c \
	server/have.c \
	server/notify.c \
	server/threads.c

lib_LTLIBRARIES = libmediatex.la
libmediatex_la_SOURCES = $(mediatex_sources)
libmediatex_la_LDFLAGS = @LIBINTL@ -pthread -lm -lavl -lcrypto

noinst_LIBRARIES = \
	client/libclient.a \
	server/libserver.a

client_libclient_a_SOURCES = \
	$(client_sources)

server_libserver_a_SOURCES = \
	$(server_sources)

# theses headers are not distributed to final user
check_HEADERS = \
	mediatex-config.h \
	misc/alloc.h \
	$(parser_headers) \
	$(client_headers) \
	$(server_headers)


headersdir = $(includedir)/mediatex
dist_headers_HEADERS = \
	mediatex-types.h \
	mediatex.h

misc_headersdir = $(headersdir)/misc
dist_misc_headers_HEADERS = \
	$(misc_headers)

memory_headersdir = $(headersdir)/memory
dist_memory_headers_HEADERS = \
	$(memory_headers)

common_headersdir = $(headersdir)/common
dist_common_headers_HEADERS = \
	$(common_headers)

client_headersdir = $(headersdir)/client
dist_client_headers_HEADERS = \
	client/commonHtml.h \
	client/catalogHtml.h \
	client/extractHtml.h

BUILT_SOURCES = \
	parser/supportFile.h \
	parser/supportFile.tab.h \
	parser/confFile.h \
	parser/confFile.tab.h \
	parser/serverFile.h \
	parser/serverFile.tab.h \
	parser/extractFile.h \
	parser/extractFile.tab.h \
	parser/catalogFile.h \
	parser/catalogFile.tab.h \
	parser/recordFile.h \
	parser/recordFile.tab.h \
	parser/shellQuery.h \
	parser/shellQuery.tab.h

cgidir = $(datarootdir)@mdtx_mediatexdir@/cgi-dir
cgi_PROGRAMS = cgi

bin_PROGRAMS = \
	mediatex \
	mediatexd

cgi_SOURCE = cgi.c
cgi_LDADD = libmediatex.la
cgi_LDFLAGS = @LIBINTL@ -lpthread -lm -lavl -lcrypto

mediatex_SOURCE = mediatex.c
mediatex_LDADD = libmediatex.la client/libclient.a
mediatex_LDFLAGS = @LIBINTL@ -lpthread -lm -lavl -lcrypto

mediatexd_SOURCE = mediatexd.c
mediatexd_LDADD = libmediatex.la server/libserver.a
mediatexd_LDFLAGS = @LIBINTL@ -lpthread -lm -lavl -lcrypto

%.tab.h %.tab.c: %.y
	$(YACC) $<

%.h %.c: %.l
	$(LEX) $< 

# stupid test bellow, but needed by distcheck
if SETUID_INSTALL
install-exec-hook:
	/bin/chmod 4755 $(DESTDIR)$(bindir)/mediatex
endif

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
ACLOCAL_AMFLAGS= -I m4
