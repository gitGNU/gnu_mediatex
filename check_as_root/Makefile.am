#  * Version: $Id: Makefile.am,v 1.2 2015/07/03 11:53:59 nroche Exp $
#  * Project: MediaTex
#  * Module: unitary tests
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

check_PROGRAMS = \
	misc/utsetuid

TESTS = \
	misc/setuid.sh \
	scripts/utusers.sh \
	scripts/uthtdocs.sh \
	scripts/utssh.sh \
	scripts/utcvs.sh \
	scripts/utgit.sh \
	scripts/utjail.sh

dist_check_SCRIPTS = \
	$(TESTS) \
	misc/user.sh

dist_check_DATA = \
	misc/setuid.exp \
	scripts/users.exp \
	scripts/htdocs.exp \
	scripts/ssh.exp \
	scripts/cvs.exp \
	scripts/git.exp \
	scripts/jail.exp

AM_CFLAGS = -DutMAIN -I $(top_srcdir)/src

misc_utsetuid_SOURCES = misc/utsetuid.c
misc_utsetuid_LDADD = ../src/.libs/libmediatex.a

all:

clean-local:
	@rm -f  misc/*.out \
		scripts/*.out

