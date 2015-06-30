dnl  * Version: $Id: configure.ac,v 1.4 2015/06/30 17:36:57 nroche Exp $
dnl  * Project: MediaTex
dnl  * Module:  autotools
dnl  *
dnl  * Process this file with autoconf to produce a configure script.
dnl 
dnl  MediaTex is an Electronic Records Management System
dnl  Copyright (C) 2014 2015 Nicolas Roche
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

AC_INIT([mediatex], [0.4], [nroche@narval.tk])
AM_INIT_AUTOMAKE([subdir-objects -Wall silent-rules])

# i18n support from GNU gettext.
AM_GNU_GETTEXT_VERSION([0.19])
AM_GNU_GETTEXT([external])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
LT_INIT
AC_PROG_AWK
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_LN_S

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/mediatex.h])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
	Makefile
	doc/mediatex-figures/Makefile
	doc/Makefile
	misc/Makefile
	etc/Makefile
	scripts/Makefile
	po/Makefile.in
	src/Makefile
	check/Makefile
	check_as_root/Makefile
	man/Makefile
	examples/Makefile
	])

# fully expand the pathnames before they go into the config.h file
mdtx_save_prefix=$prefix
test "x$prefix" = "xNONE" && prefix=$ac_default_prefix
AC_DEFINE_UNQUOTED(CONF_PREFIX, "$prefix",
                   [prefix for mdtx installation (/)])

mdtx_save_exec_prefix=$exec_prefix
test "x$exec_prefix" = "xNONE" && exec_prefix=$prefix
AC_DEFINE_UNQUOTED(CONF_EXEC_PREFIX, "$exec_prefix",
                   [prefix for mdtx libraries installation (prefix)])

eval mdtx_bindir=`echo $bindir`
AC_DEFINE_UNQUOTED(CONF_BINDIR, "$mdtx_bindir",
                   [directory for mdtx scripts (exec_prefix/bin)])

eval mdtx_sysconfdir=`echo $sysconfdir`
AC_DEFINE_UNQUOTED(CONF_SYSCONFDIR, "$mdtx_sysconfdir",
                   [directory for mdtx configuration (prefix/etc)])

eval mdtx_localstatedir=`echo $localstatedir`
AC_DEFINE_UNQUOTED(CONF_LOCALSTATEDIR, "$mdtx_localstatedir",
                   [directory for mdtx internal files (prefix/var)])

eval mdtx_datarootdir=`echo $datarootdir`
AC_DEFINE_UNQUOTED(CONF_DATAROOTDIR, "$mdtx_datarootdir",
                   [directory for mdtx internal files (prefix/share)])

prefix=$mdtx_save_prefix
exec_prefix=$mdtx_save_exec_prefix

mdtx_mediatexdir="/mediatex"
AC_SUBST(mdtx_mediatexdir)	       
AC_DEFINE_UNQUOTED(CONF_MEDIATEXDIR, "$mdtx_mediatexdir", 
		  [generic directory name to use])

# Add option to disable setuid during install, use in distcheck
AC_ARG_ENABLE(setuid-install, 
 AS_HELP_STRING(
 [--disable-setuid-install       do not set setuid flags during install]), 
 [enable_setuid_install=$enableval], [enable_setuid_install="yes"])
AM_CONDITIONAL(SETUID_INSTALL, test x"$enable_setuid_install" = "xyes")

# Documentation figures (doc/mediatex-figures/Makefile.am)
EPS_FILES=$(for IMG in $(ls $srcdir/doc/mediatex-figures/*.fig); do \
		IMG=${IMG#$srcdir/doc/mediatex-figures/}; \
		echo -n "${IMG%.fig}.eps "; \
		done)
PNG_FILES=$(for IMG in $(ls $srcdir/doc/mediatex-figures/*.fig); do \
		IMG=${IMG#$srcdir/doc/mediatex-figures/}; \
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