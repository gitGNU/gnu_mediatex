#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: unitTest.sh,v 1.2 2014/11/13 16:36:13 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * Execute non-regression tests on modules :
#   "module.out" output is match against "module.exp" expected output
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014  Nicolas Roche
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#=======================================================================

# includes
MDTX_UNIT_TEST_SH=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir

# this function populate fake DATADIR used for unit tests
function UNIT_TEST_populate_datadir()
{
    EXAMPLE=$DATADIR/examples
    mkdir -p $EXAMPLE

    for f in mediatex_cron mediatex.conf supports.txt logo.png \
	catalog00.txt extract00.txt servers.txt htgroup \
	mediatex.css floppy-icon.png \
	viewvc.conf apache-mdtx.conf header.ezt \
	; do
	install -m 444 $srcdir/../../examples/$f $EXAMPLE
    done
    for t in home index cache score cgi; do
	install -m 444 $srcdir/../../examples/$t.htaccess $EXAMPLE
    done

    mkdir -p $ETCDIR # TO MOVE !!
    mkdir -p $SYSCONFDIR/apache2/conf.d
}
# this function set the directory to use for unit tests
# $1: root directory to add before /
function UNIT_TEST_push_root_directory()
{
    if [ $# -ne 1 ]; then
	echo "expect 1 parameter"
	exit 1;
    fi

    # overwrite global variables
    BINDIR="${1}${BINDIR}"
    DATAROOTDIR="${1}${DATAROOTDIR}"
    EXEC_PREFIX="${1}${EXEC_PREFIX}"
    LIBDIR="${1}${LIBDIR}"
    LOCALSTATEDIR="${1}${LOCALSTATEDIR}"
    PREFIX="${1}${PREFIX}"
    SYSCONFDIR="${1}${SYSCONFDIR}"

    mkdir -p $SYSCONFDIR/cron.d
    
    # re-define scripts variables 
    ETCDIR="${SYSCONFDIR}${MEDIATEX}"
    DATADIR="${DATAROOTDIR}${MEDIATEX}"
    STATEDIR="${LOCALSTATEDIR}/lib${MEDIATEX}"
    CACHEDIR="${LOCALSTATEDIR}/cache${MEDIATEX}"

    UNIT_TEST_populate_datadir
}

# so as to pass unit test easier
# $1: out file to sed
function UNIT_TEST_mrProper()
{
    # hide line numbers
    sed $1 -i -e "s,^\(\[.*\):.*\],\1],"
    
    # hide tmp dir name
    sed $1 -i -e "s|/var/ut/.\{3\}/|/|"

    # hide user and group numbers
    sed $1 -i -e "s,^\(.*:x:\).*:,\1XXX:,"

    # hide commit date and id
    sed $1 -i -e "s,\(date:\) .*;  a,\1 XXX;  a,"
    sed $1 -i -e "s,\(commitid:\) .*;,\1 XXX;,"

    # hide group number
    sed $1 -i -e "s,:x:[[:digit:]]*:,:x:XXX:,"
}

# this function switch on into the unit test mode
# $1 the related module file name
# return 1 if not called by the module
function UNIT_TEST_start()
{
    [ -z $1 ] && echo "Please provide module file name"

    # this return when module is sourced
    [ $(basename "$0") = "$1.sh" ] || return 1
    UNIT_TEST_RUNNING=1
    if [ $(id -u) -ne 0 ]; then
	echo -n "(root needed for this test) "
	exit 0
    fi

    # log to stderr for tests
    LOG_FACILITY=file
    LOG_SEVERITY=debug

    # root directory for tests. Note:
    # jail dir must be owned by root and not writable by others users,
    # both conditions must be recursively applied from / dir.
    mkdir -p /var/ut
    UNIT_TEST_ROOTDIR=$(mktemp -d /var/ut/XXX)
    chmod 755 $UNIT_TEST_ROOTDIR
    UNIT_TEST_push_root_directory $UNIT_TEST_ROOTDIR

    # backup stdout and stderr values
    exec 3>&1 4>&2
    
    # redirect outputs to a file
    exec >${1}.out 2>&1
}

# this function switch on into the unit test mode
# $1 the module file name provided by the caller
function UNIT_TEST_stop()
{
    [ -z $1 ] && echo "Please provide module file name"

    # flush output file (not needed)
    exec >&- 2>&-

    # retrieve the originals outputs
    exec >&3 2>&4

    # compare with the expected output
    UNIT_TEST_EXPECTED="$srcdir/${1}.exp"
    [ -f $UNIT_TEST_EXPECTED ] || UNIT_TEST_EXPECTED="/dev/null"
    UNIT_TEST_mrProper ${1}.out
    diff $UNIT_TEST_EXPECTED ${1}.out
    rm -fr $UNIT_TEST_ROOTDIR
}

#main
UNIT_TEST_RUNNING=0

# unit test
if UNIT_TEST_start "unitTest"; then
    echo "Such stanza are not executed by scripts that source modules."
    echo "This should only appears into the unitTests.out file."
    UNIT_TEST_stop "unitTest"
fi
