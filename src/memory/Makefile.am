#  * Version: $Id: Makefile.am,v 1.2 2014/11/13 16:36:26 nroche Exp $
#  * Project: MediaTex
#  * Module:  memory tree modules
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

headersdir = $(includedir)/mediatex/memory
dist_headers_HEADERS = $(memory_headers)

check_PROGRAMS = \
	utstrdsm utardsm \
	utcvsPrint \
	utconfTree \
	utsupportTree \
	utarchive \
	utcatalogTree \
	utserverTree \
	utextractTree \
	utrecordTree \
	utcacheTree

TESTS = \
	utStrdsm.sh utArdsm.sh \
	utCvsPrint.sh \
	utConfTree.sh \
	utSupportTree.sh \
	utArchive.sh \
	utCatalogTree.sh \
	utServerTree.sh \
	utExtractTree.sh \
	utRecordTree.sh \
	utCacheTree.sh

dist_check_SCRIPTS = $(TESTS)
dist_check_DATA = \
	utStrdsm.exp utArdsm.exp \
	utCvsPrint.exp \
	utConfTree.exp utConfTree.exp2 \
	utSupportTree.exp utSupportTree.exp2 \
	utArchive.exp \
	utCatalogTree.exp utCatalogTree.exp2 \
	utServerTree.exp utServerTree.exp2 \
	utExtractTree.exp utExtractTree.exp2 \
	utRecordTree.exp utRecordTree.aes utRecordTree.exp2 \
	utCacheTree.exp \
	hostKey_rsa hostKey_rsa.pub \
	user1Key_rsa user1Key_rsa.pub \
	user2Key_rsa user2Key_rsa.pub \
	user3Key_dsa user3Key_dsa.pub

check_LIBRARIES = libutmemory.a libmemory.a
libutmemory_a_SOURCES = utFunc.c utFunc.h
libmemory_a_SOURCES = $(memory_sources)
objects = ../misc/libmisc.a libutmemory.a

utstrdsm_SOURCES = strdsm.c strdsm.h
utstrdsm_CFLAGS = -DutMAIN
utstrdsm_LDADD = $(objects) 
utstrdsm_LDFLAGS = -pthread

utardsm_SOURCES = ardsm.c ardsm.h strdsm.h 
utardsm_CFLAGS = -DutMAIN
utardsm_LDADD = strdsm.o  $(objects)
utardsm_LDFLAGS = -pthread

utcvsPrint_SOURCES = cvsPrint.c cvsPrint.h strdsm.h
utcvsPrint_CFLAGS = -DutMAIN
utcvsPrint_LDADD = strdsm.o $(objects)
utcvsPrint_LDFLAGS = -pthread

utconfTree_SOURCES = confTree.c confTree.h \
	strdsm.h ardsm.h
utconfTree_CFLAGS = -DutMAIN
utconfTree_LDADD = extractTree.o catalogTree.o serverTree.o \
 recordTree.o supportTree.o archive.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utconfTree_LDFLAGS = -pthread -lavl

utsupportTree_SOURCES = supportTree.c supportTree.h \
	strdsm.h ardsm.h
utsupportTree_CFLAGS = -DutMAIN
utsupportTree_LDADD = extractTree.o catalogTree.o serverTree.o \
 recordTree.o confTree.o archive.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utsupportTree_LDFLAGS = -pthread -lavl

utarchive_SOURCES = archive.c archive.h confTree.h \
	strdsm.h ardsm.h
utarchive_CFLAGS = -DutMAIN
utarchive_LDADD = extractTree.o catalogTree.o serverTree.o \
 recordTree.o supportTree.o confTree.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utarchive_LDFLAGS = -pthread -lavl

utcatalogTree_SOURCES = catalogTree.c catalogTree.h \
	strdsm.h ardsm.h
utcatalogTree_CFLAGS = -DutMAIN
utcatalogTree_LDADD = extractTree.o serverTree.o recordTree.o \
 supportTree.o confTree.o archive.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects) 
utcatalogTree_LDFLAGS = -pthread -lavl

utserverTree_SOURCES = serverTree.c serverTree.h \
	strdsm.h ardsm.h
utserverTree_CFLAGS = -DutMAIN
utserverTree_LDADD = extractTree.o catalogTree.o recordTree.o \
 supportTree.o confTree.o archive.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utserverTree_LDFLAGS = -pthread -lavl

utextractTree_SOURCES = extractTree.c extractTree.h \
	strdsm.h ardsm.h
utextractTree_CFLAGS = -DutMAIN
utextractTree_LDADD = catalogTree.o serverTree.o recordTree.o \
 supportTree.o confTree.o archive.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utextractTree_LDFLAGS = -pthread -lavl

utrecordTree_SOURCES = recordTree.c recordTree.h archive.h \
	strdsm.h ardsm.h
utrecordTree_CFLAGS = -DutMAIN
utrecordTree_LDADD = extractTree.o catalogTree.o serverTree.o \
 supportTree.o confTree.o archive.o cacheTree.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utrecordTree_LDFLAGS = -pthread -lavl

utcacheTree_SOURCES = cacheTree.c cacheTree.h \
	strdsm.h ardsm.h
utcacheTree_CFLAGS = -DutMAIN
utcacheTree_LDADD = extractTree.o catalogTree.o serverTree.o \
 recordTree.o supportTree.o confTree.o archive.o \
 cvsPrint.o strdsm.o ardsm.o $(objects)
utcacheTree_LDFLAGS = -pthread -lavl

clean-local:
	-rm -f ut.out ut.txt

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent