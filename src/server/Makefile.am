#  * Project: MediaTex
#  * Module:  server modules
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

check_PROGRAMS = \
	utcache  \
	utdeliver \
	utextract \
	utcgiSrv \
	uthave \
	utnotify \
	utthreads

TESTS = \
	utCache.sh \
	utDeliver.sh \
	utExtract.sh \
	utCgiSrv.sh \
	utHave.sh \
	utNotify.sh \
	utThreads.sh

dist_check_SCRIPTS = $(TESTS)
dist_check_DATA = \
	utCache.exp \
	utDeliver.exp \
	utExtract.exp \
	utCgiSrv.exp \
	utHave.exp \
	utNotify.exp \
	utThreads.exp

check_LIBRARIES = libutserver.a
noinst_LIBRARIES = libserver.a 
libutserver_a_SOURCES = utFunc.c utFunc.h
libserver_a_SOURCES = $(server_sources)

objects = \
	libutserver.a \
	../common/libcommon.a \
	../parser/libparser.a \
	../memory/libmemory.a \
	../misc/libmisc.a

utcache_SOURCES = cache.c cache.h 
utcache_CFLAGS = -DutMAIN
utcache_LDADD = $(objects)
utcache_LDFLAGS = -pthread -lm -lavl -lcrypto

utdeliver_SOURCES = deliver.c deliver.h
utdeliver_CFLAGS = -DutMAIN
utdeliver_LDADD = cache.o $(objects)
utdeliver_LDFLAGS = -pthread -lm -lavl -lcrypto

utextract_SOURCES = extract.c extract.h deliver.h cache.h
utextract_CFLAGS = -DutMAIN
utextract_LDADD = deliver.o cache.o $(objects)
utextract_LDFLAGS = -pthread -lm -lavl -lcrypto

utcgiSrv_SOURCES = cgiSrv.c cgiSrv.h extract.h deliver.h cache.h
utcgiSrv_CFLAGS = -DutMAIN
utcgiSrv_LDADD = extract.o deliver.o  cache.o $(objects)
utcgiSrv_LDFLAGS = -pthread -lm -lavl -lcrypto

uthave_SOURCES = have.c have.h extract.h deliver.h cache.h
uthave_CFLAGS = -DutMAIN
uthave_LDADD = extract.o deliver.o cache.o $(objects)
uthave_LDFLAGS = -pthread -lm -lavl -lcrypto

utnotify_SOURCES = notify.c notify.h extract.h deliver.h cache.h
utnotify_CFLAGS = -DutMAIN
utnotify_LDADD = extract.o deliver.o cache.o $(objects)
utnotify_LDFLAGS = -pthread -lm -lavl -lcrypto

utthreads_SOURCES = threads.c threads.h
utthreads_CFLAGS = -DutMAIN
utthreads_LDADD = $(objects)
utthreads_LDFLAGS = -pthread -lm -lavl -lcrypto

clean-local:
	-rm -f ut.out ut*.sort mediatexd

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent
