#  * Version: $Id: Makefile.am,v 1.2 2014/11/13 16:36:36 nroche Exp $
#  * Project: MediaTex
#  * Module:  miscellaneous modules
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

headersdir = $(includedir)/mediatex/misc
dist_headers_HEADERS = $(misc_headers)

check_PROGRAMS = \
	utgetcgivars \
	utlog \
	utalloc \
	utcommand utsetuid \
	utsignals \
	utprogbar utdevice utmd5sum \
	utaddress uttcp \
	utcypher \
	utshm \
	utperm \
	utkeys \
	utlocks \
	uthtml

TESTS = \
	utGetcgivars.sh \
	utLog.sh \
	utCommand.sh utSetuid.sh\
	utSignals.sh \
	utChecksums.sh \
	utAddress.sh utTcp.sh \
	utCypher.sh \
	utShm.sh \
	utPerm.sh \
	utKeys.sh \
	utLocks.sh \
	utHtml.sh

dist_check_SCRIPTS = $(TESTS) env.sh
dist_check_DATA = \
	utGetcgivars.exp \
	utLog.exp \
	utCommand.exp utSetuid.exp \
	utSignals.exp \
	utChecksums.exp \
	utAddress.exp utTcp.exp \
	utCypher.exp \
	utShm.exp \
	utPerm.exp \
	utLocks.exp \
	utHtml.exp utHtml.exp2

# will be used later by other source directories
check_LIBRARIES = libmisc.a
libmisc_a_SOURCES = $(misc_sources)

utgetcgivars_SOURCES = getcgivars.c getcgivars.h
utgetcgivars_CFLAGS = -DutMAIN

utlog_SOURCES = log.c log.h
utlog_CFLAGS = -DutMAIN

utalloc_SOURCES = alloc.c alloc.h log.h
utalloc_CFLAGS = -DutMAIN
utalloc_LDADD = log.o

utcommand_SOURCES = command.c command.h alloc.h log.h
utcommand_CFLAGS = -DutMAIN
utcommand_LDADD = alloc.o log.o

utsetuid_SOURCES = setuid.c setuid.h command.h alloc.h log.h
utsetuid_CFLAGS = -DutMAIN
utsetuid_LDADD = command.o alloc.o log.o
utsetuid_LDFLAGS = -pthread

utsignals_SOURCES = signals.c signals.h command.h alloc.h log.h
utsignals_CFLAGS = -DutMAIN
utsignals_LDADD = setuid.o command.o alloc.o log.o 
utsignals_LDFLAGS = -pthread

utprogbar_SOURCES = progbar.c progbar.h
utprogbar_CFLAGS = -DutMAIN

utdevice_SOURCES = device.c device.h command.h alloc.h log.h
utdevice_CFLAGS = -DutMAIN
utdevice_LDADD = setuid.o command.o alloc.o log.o 

utmd5sum_SOURCES = md5sum.c md5sum.h device.h progbar.h \
	command.h alloc.h log.h
utmd5sum_CFLAGS = -DutMAIN
utmd5sum_LDADD =  device.o progbar.o setuid.o command.o alloc.o log.o

utaddress_SOURCES = address.c address.h command.h alloc.h log.h
utaddress_CFLAGS = -DutMAIN
utaddress_LDADD = setuid.o command.o alloc.o log.o

uttcp_SOURCES = tcp.c tcp.h address.h command.h alloc.h log.h
uttcp_CFLAGS = -DutMAIN
uttcp_LDADD = address.o setuid.o command.o alloc.o log.o

utcypher_SOURCES = cypher.c cypher.h command.h alloc.h log.h
utcypher_CFLAGS = -DutMAIN
utcypher_LDADD =  setuid.o command.o alloc.o log.o

utshm_SOURCES = shm.c shm.h command.h alloc.h log.h
utshm_CFLAGS = -DutMAIN
utshm_LDADD = setuid.o command.o alloc.o log.o 

utperm_SOURCES = perm.c perm.h setuid.h command.h alloc.h log.h
utperm_CFLAGS = -DutMAIN
utperm_LDADD = setuid.o command.o alloc.o log.o

utkeys_SOURCES = keys.c keys.h setuid.h command.h alloc.h log.h
utkeys_CFLAGS = -DutMAIN 
utkeys_LDADD = setuid.o command.o alloc.o log.o 

utlocks_SOURCES = locks.c locks.h command.h alloc.h log.h
utlocks_CFLAGS = -DutMAIN
utlocks_LDADD = signals.o setuid.o command.o alloc.o log.o
utlocks_LDFLAGS = -pthread

uthtml_SOURCES = html.c html.h command.h alloc.h log.h
uthtml_CFLAGS = -DutMAIN
uthtml_LDADD = setuid.o command.o alloc.o log.o

clean-local:
	-rm -f ut.out ut.txt ut.html ut.css key key.pub 

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LIBTOOL = @LIBTOOL@ --silent