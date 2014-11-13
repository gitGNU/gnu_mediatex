#  * Version: $Id: Makefile.am,v 1.2 2014/11/13 16:36:16 nroche Exp $
#  * Project: MediaTex
#  * Module:  client modules (User API)
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

headersdir = $(includedir)/mediatex/client
dist_headers_HEADERS = $(client_headers)

check_PROGRAMS = \
	utserv \
	utconf \
	utsupp \
	utmotd \
	utcommonHtml \
	utcatalogHtml \
	utextractHtml \
	utmisc \
	utshellQuery utshellQuery.tab

TESTS = \
	utServ.sh \
	utConf.sh \
	utSupp.sh \
	utMotd.sh \
	utCommonHtml.sh \
	utCatalogHtml.sh \
	utExtractHtml.sh \
	utMisc.sh \
	utShellQuery.sh

dist_check_SCRIPTS = $(TESTS)
dist_check_DATA = \
	utServ.exp \
	utSupp.exp \
	utConf.exp \
	utMotd.exp \
	utCommonHtml.exp \
	utCatalogHtml.exp \
	utExtractHtml.exp \
	utMisc.exp \
	utShellQuery.exp

objects = \
	../common/libcommon.a \
	../parser/libparser.a \
	../memory/libmemory.a \
	../misc/libmisc.a 

utserv_SOURCES = serv.c serv.h
utserv_CFLAGS = -DutMAIN
utserv_LDADD = $(objects)
utserv_LDFLAGS = -pthread -lm -lavl

utsupp_SOURCES = supp.c supp.h
utsupp_CFLAGS = -DutMAIN
utsupp_LDADD = $(objects)
utsupp_LDFLAGS = -pthread -lm -lavl

utconf_SOURCES = conf.c conf.h serv.h
utconf_CFLAGS = -DutMAIN
utconf_LDADD = serv.o $(objects)
utconf_LDFLAGS = -pthread -lm -lavl

utmotd_SOURCES = motd.c motd.h serv.h supp.h conf.h
utmotd_CFLAGS = -DutMAIN
utmotd_LDADD = serv.o conf.o supp.o $(objects)
utmotd_LDFLAGS = -pthread -lm -lavl

utcommonHtml_SOURCES = commonHtml.c commonHtml.h
utcommonHtml_CFLAGS = -DutMAIN
utcommonHtml_LDADD = $(objects)
utcommonHtml_LDFLAGS = -pthread -lm -lavl

utcatalogHtml_SOURCES = catalogHtml.c catalogHtml.h commonHtml.h
utcatalogHtml_CFLAGS = -DutMAIN
utcatalogHtml_LDADD = commonHtml.o $(objects)
utcatalogHtml_LDFLAGS = -pthread -lm -lavl

utextractHtml_SOURCES = extractHtml.c extractHtml.h commonHtml.h
utextractHtml_CFLAGS = -DutMAIN
utextractHtml_LDADD = commonHtml.o $(objects)
utextractHtml_LDFLAGS = -pthread -lm -lavl

utmisc_SOURCES = misc.c misc.h
utmisc_CFLAGS = -DutMAIN
utmisc_LDADD = commonHtml.o extractHtml.o catalogHtml.o $(objects)
utmisc_LDFLAGS = -pthread -lm -lavl

utshellQuery_CFLAGS = -DutMAIN
utshellQuery_SOURCES = shellQuery.c shellQuery.h
utshellQuery_LDADD = $(objects)
utshellQuery_LDFLAGS = -pthread -lm -lavl

utshellQuery_tab_CFLAGS = -DutMAIN
utshellQuery_tab_SOURCES = shellQuery.tab.c shellQuery.tab.h
utshellQuery_tab_LDADD = shellQuery.o $(objects)
utshellQuery_tab_LDFLAGS = -pthread -lm -lavl

%.tab.h %.tab.c: %.y %.h
	$(YACC) $<

%.h %.c: %.l
	$(LEX) $(LFLAGS) $< 

all-local: \
	shellQuery.h \
	shellQuery.tab.h \
	shellQuery.c \
	shellQuery.tab.c 

clean-local:
	-rm -f ut.{out,output} \
		user1Key_rsa.pub user3Key_dsa.pub ut.out2

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent