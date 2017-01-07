dnl  * Project: Jukebox
dnl  * Module:  autotools
dnl  *
dnl  * Process this file with autoconf to produce a configure script.
dnl 
dnl  Jukebox is an Electronic Records Management System
dnl  Copyright (C) 2017 Nicolas Roche
dnl 
dnl  This program is free software: you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation, either version 3 of the License, or
dnl  any later version.
dnl 
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl 
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([jukebox], [0.1], [nroche@narval.fr.eu.org])
AM_INIT_AUTOMAKE()

## i18n support from GNU gettext.
#AM_GNU_GETTEXT_VERSION([0.19])
#AM_GNU_GETTEXT([external])

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC

AC_PREREQ

#	po/Makefile.in
AC_CONFIG_FILES([
	Makefile
	doc/jukebox-figures/Makefile
	doc/Makefile
	check/Makefile
	man/Makefile
	])

# Documentation figures (doc/jukebox-figures/Makefile.am)
EPS_FILES=$(for IMG in $(ls $srcdir/doc/jukebox-figures/*.fig); do \
		IMG=${IMG#$srcdir/doc/jukebox-figures/}; \
		echo -n "${IMG%.fig}.eps "; \
		done)
PNG_FILES=$(for IMG in $(ls $srcdir/doc/jukebox-figures/*.fig); do \
		IMG=${IMG#$srcdir/doc/jukebox-figures/}; \
		echo -n "${IMG%.fig}.png "; \
		done)
AC_SUBST([EPS_FILES]) 
AC_SUBST([PNG_FILES]) 

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

AC_OUTPUT
