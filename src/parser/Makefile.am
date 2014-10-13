#  * Version: $Id: Makefile.am,v 1.1 2014/10/13 19:39:46 nroche Exp $
#  * Project: MediaTex
#  * Module:  parser modules
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

headersdir = $(includedir)/mediatex/parser
dist_headers_HEADERS = $(parser_headers)

check_PROGRAMS = \
	utsupportFile utsupportFile.tab \
	utconfFile utconfFile.tab \
	utserverFile utserverFile.tab \
	utextractFile utextractFile.tab \
	utcatalogFile utcatalogFile.tab \
	utrecordList utrecordList.tab

TESTS = \
	utSupportFile.sh \
	utConfFile.sh \
	utServerFile.sh \
	utCatalogFile.sh \
	utExtractFile.sh \
	utRecordList.sh

dist_check_SCRIPTS = $(TESTS)

check_LIBRARIES = libparser.a
libparser_a_SOURCES = $(parser_sources)

objects = \
	../memory/libmemory.a \
	../misc/libmisc.a

utsupportFile_CFLAGS = -DutMAIN
utsupportFile_SOURCES = supportFile.c supportFile.h
utsupportFile_LDADD = $(objects)
utsupportFile_LDFLAGS = -pthread -lavl

utsupportFile_tab_CFLAGS = -DutMAIN
utsupportFile_tab_SOURCES = supportFile.tab.c supportFile.tab.h
utsupportFile_tab_LDADD = supportFile.o $(objects)
utsupportFile_tab_LDFLAGS = -pthread -lavl

utconfFile_CFLAGS = -DutMAIN
utconfFile_SOURCES = confFile.c confFile.h
utconfFile_LDADD = $(objects)
utconfFile_LDFLAGS = -pthread -lavl

utconfFile_tab_CFLAGS = -DutMAIN
utconfFile_tab_SOURCES = confFile.tab.c confFile.tab.h
utconfFile_tab_LDADD = confFile.o supportFile.o supportFile.tab.o \
	$(objects)
utconfFile_tab_LDFLAGS = -pthread -lavl

utserverFile_CFLAGS = -DutMAIN
utserverFile_SOURCES = serverFile.c serverFile.h
utserverFile_LDADD = $(objects)
utserverFile_LDFLAGS = -pthread -lavl

utserverFile_tab_CFLAGS = -DutMAIN
utserverFile_tab_SOURCES = serverFile.tab.c serverFile.tab.h
utserverFile_tab_LDADD = serverFile.o confFile.tab.o confFile.o \
 supportFile.o supportFile.tab.o $(objects)
utserverFile_tab_LDFLAGS = -pthread -lavl

utextractFile_CFLAGS = -DutMAIN
utextractFile_SOURCES = extractFile.c extractFile.h
utextractFile_LDADD = $(objects)
utextractFile_LDFLAGS = -pthread -lavl

utextractFile_tab_CFLAGS = -DutMAIN
utextractFile_tab_SOURCES = extractFile.tab.c extractFile.tab.h
utextractFile_tab_LDADD = extractFile.o confFile.tab.o confFile.o \
	$(objects)
utextractFile_tab_LDFLAGS = -pthread -lavl

utcatalogFile_CFLAGS = -DutMAIN
utcatalogFile_SOURCES = catalogFile.c catalogFile.h
utcatalogFile_LDADD = $(objects)
utcatalogFile_LDFLAGS = -pthread -lavl

utcatalogFile_tab_CFLAGS = -DutMAIN
utcatalogFile_tab_SOURCES = catalogFile.tab.c catalogFile.tab.h
utcatalogFile_tab_LDADD = catalogFile.o confFile.tab.o confFile.o \
	$(objects)
utcatalogFile_tab_LDFLAGS = -pthread -lavl

utrecordList_CFLAGS = -DutMAIN
utrecordList_SOURCES = recordList.c recordList.h
utrecordList_LDADD = $(objects)
utrecordList_LDFLAGS = -pthread -lavl

utrecordList_tab_CFLAGS = -DutMAIN
utrecordList_tab_SOURCES = recordList.tab.c recordList.tab.h
utrecordList_tab_LDADD = recordList.o serverFile.o serverFile.tab.o \
	confFile.o confFile.tab.o $(objects)
utrecordList_tab_LDFLAGS = -pthread -lavl

%.tab.h %.tab.c: %.y %.h
	$(YACC) $<

%.h %.c: %.l
	$(LEX) $(LFLAGS) $< 

all-local: \
	$(parser_headers) \
	$(parser_sources)

clean-local:
	-rm -f ut.{out,out2,output} ut*.txt utAes.out

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent