# Process this file with automake to produce Makefile.in
# Makefile.am for hello/man.
#
# Copyright 1996, 1997, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

man_MANS = mediatex.1 mediatexd.1 mediatex.cgi.1
EXTRA_DIST = $(man_MANS)
MAINTAINERCLEANFILES = $(man_MANS)

# The man pages depend on the --help strings and the version number.
common_mandeps = $(top_srcdir)/configure.ac

# Depend on the source, not the binary; we don't need to regenerate the
# binary when any source file changes, only the main one.
# Use -o so that the `missing' program can infer the output file.

mediatex.1: $(top_srcdir)/src/mediatex $(common_mandeps)
	$(HELP2MAN) --no-discard-stderr --libtool \
		--name="Mediatex client program" \
		--info-page=mediatex \
		../src/mediatex -o $@

mediatexd.1: $(top_srcdir)/src/mediatexd $(common_mandeps)
	$(HELP2MAN) --no-discard-stderr --libtool \
		--name="Mediatex server program" \
		--info-page=mediatexd \
		../src/mediatexd -o $@

mediatex.cgi.1: $(top_srcdir)/src/mediatex-cgi $(common_mandeps)
	$(HELP2MAN) --no-discard-stderr --libtool \
		--name="Mediatex cgi program" \
		--info-page=mediatex-cgi \
		../src/mediatex-cgi -o $@

clean-local:
	rm -f *.1
