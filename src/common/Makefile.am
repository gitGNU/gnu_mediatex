#  * Project: MediaTex
#  * Module:  common modules (both used by clients and server)
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

headersdir = $(includedir)/mediatex/common
dist_headers_HEADERS = $(common_headers)

check_PROGRAMS = \
	utregister \
	utconnect \
	utssh \
	utupgrade \
	utopenClose \
	utextractScore \
	utcgi 

TESTS = \
	utRegister.sh \
	utConnect.sh \
	utSsh.sh \
	utUpgrade.sh \
	utOpenClose.sh \
	utExtractScore.sh \
	utCgi.sh

dist_check_SCRIPTS = $(TESTS)
dist_check_DATA = \
	utRegister.exp \
	utConnect.exp \
	utCgi.exp \
	utSsh.exp utSsh.exp2 utSsh.exp3 utSsh.exp4 \
	utUpgrade.exp \
	utUpgrade.in \
	utOpenClose.exp \
	utExtractScore.exp

check_LIBRARIES = libcommon.a
libcommon_a_SOURCES = $(common_sources)

objects = \
	../parser/libparser.a \
	../memory/libmemory.a \
	../misc/libmisc.a

utregister_SOURCES = register.c register.h
utregister_CFLAGS = -DutMAIN
utregister_LDADD = $(objects)
utregister_LDFLAGS = -pthread -lavl -lcrypto

utconnect_SOURCES = connect.c connect.h
utconnect_CFLAGS = -DutMAIN
utconnect_LDADD = $(objects)
utconnect_LDFLAGS = -pthread -lavl -lcrypto

utssh_SOURCES = ssh.c ssh.h
utssh_CFLAGS = -DutMAIN
utssh_LDADD = $(objects)
utssh_LDFLAGS = -pthread -lavl -lcrypto

utupgrade_SOURCES = upgrade.c upgrade.h ssh.h
utupgrade_CFLAGS = -DutMAIN
utupgrade_LDADD = ssh.o $(objects)
utupgrade_LDFLAGS = -pthread -lm -lavl -lcrypto

utopenClose_SOURCES = openClose.c openClose.h register.h upgrade.h ssh.h
utopenClose_CFLAGS = -DutMAIN
utopenClose_LDADD = register.o upgrade.o ssh.o $(objects)
utopenClose_LDFLAGS = -pthread -lm -lavl -lcrypto

utextractScore_SOURCES = extractScore.c extractScore.h \
	openClose.h upgrade.h ssh.h
utextractScore_CFLAGS = -DutMAIN
utextractScore_LDADD = openClose.o register.o upgrade.o ssh.o $(objects)
utextractScore_LDFLAGS = -pthread -lm -lavl -lcrypto

utcgi_SOURCES = cgi.c
utcgi_CFLAGS = -DutMAIN
utcgi_LDADD = libcommon.a $(objects)
utcgi_LDFLAGS = -pthread -lm -lavl -lcrypto

clean-local:
	-rm -f ut.out ut*.sort

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent
