#  * Project: Jukebox
#  * Module: unitary tests
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2017 Nicolas Roche
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

TESTS = \
	test1.sh \
	test2.sh \
	test3.sh \
	test4.sh \
	test5.sh

dist_check_SCRIPTS = \
	test.sh \
	$(TESTS)

dist_check_DATA = \
	data/cow1.txt \
	data/cow2.txt \
	data/cow_03.cat \
	data/eye.txt \
	data/gnu.txt \
	test1.conf \
	test1.exp \
	test1.extr \
	test1.motd \
	test1.supp \
	test2.conf \
	test2.exp \
	test2.extr \
	test2.motd \
	test2.supp \
	test3.conf \
	test3.exp \
	test3.extr \
	test3.motd \
	test3.supp \
	test4.conf \
	test4.exp \
	test4.extr \
	test4.motd \
	test4.supp \
	test5.conf \
	test5.exp \
	test5.extr \
	test5.motd \
	test5.supp

clean-local:
	@rm -f *.out
	@rm -fr tmp/
