#!/bin/bash
#=======================================================================
# * Version: $Id: utscripts.sh,v 1.2 2015/07/03 16:02:12 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * Unit test functions
# * 
# * Note: need to use '/' for jails, so differ from utmediatex.sh
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014 2015 2016 Nicolas Roche
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
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# this function populate fake DATADIR used for unit tests
function UNIT_TEST_populate_datadir()
{
    mkdir -p $MISC

    for f in mediatex_cron mediatex.conf supports.txt logo \
	catalog000.txt extract000.txt servers.txt htgroup \
	mediatex.css floppy-icon.png \
	viewvc.conf apache-mdtx.conf header.ezt \
	; do
	install -m 444 $srcdir/../misc/$f $MISC
    done
    for t in home index cache score cgi; do
	install -m 444 $srcdir/../misc/$t.htaccess $MISC
    done

    # Note:
    #  Jail dir must be owned by root and not writable by others users,
    #  both conditions must be recursively applied from / dir.
    mkdir -p $UNIT_TEST_ROOTDIR
    chmod 755 $UNIT_TEST_ROOTDIR

    mkdir -p $ETCDIR # TO MOVE !!
    mkdir -p $SYSCONFDIR/apache2/conf-available
    mkdir -p $SYSCONFDIR/cron.d
}

# this function set the directory to use for unit tests. 
# $1: UNIT_TEST_ROOTDIR
function UNIT_TEST_push_root_directory()
{
    if [ $# -ne 1 ]; then
	echo "expect 1 parameter"
	exit 1;
    fi

    # path configuration variables for unit tests: 
    #  we use "/tmp/ut-XXX" to prefix all paths
    

    # overwrite global variables
    BINDIR="${1}${BINDIR}"
    DATAROOTDIR="${1}${DATAROOTDIR}"
    EXEC_PREFIX="${1}${EXEC_PREFIX}"
    LIBDIR="${1}${LIBDIR}"
    LOCALSTATEDIR="${1}${LOCALSTATEDIR}"
    PREFIX="${1}${PREFIX}"
    SYSCONFDIR="${1}${SYSCONFDIR}"

    # overwrite scripts variables 
    ETCDIR="${1}${ETCDIR}"
    DATADIR="${1}${DATADIR}"
    STATEDIR="${1}${STATEDIR}"
    CACHEDIR="${1}${CACHEDIR}"
    PIDDIR="${1}${PIDDIR}"
    SCRIPTS="${1}${SCRIPTS}"
    MISC="${1}${MISC}"
    HOSTSSH="${1}${HOSTSSH}"

    # overwrite recurent variables
    CVSROOT="${1}${CVSROOT}"
    MDTXHOME="${1}${MDTXHOME}"
    MD5SUMS="${1}${MD5SUMS}"
    CACHES="${1}${CACHES}"
    EXTRACT="${1}${EXTRACT}"
    CVSCLT="${1}${CVSCLT}"
    JAIL="${1}${JAIL}"
    HOMES="${1}${HOMES}"
    HTDOCS="${1}${HTDOCS}"
    MDTXCVS="${1}${MDTXCVS}"
    CONFFILE="${1}${CONFFILE}"
    PIDFILE="${1}${PIDFILE}"

    UNIT_TEST_populate_datadir
}

# so as to pass unit test easier
# $1: out file to sed
function UNIT_TEST_mrProper()
{
    # hide line numbers
    sed $1 -i -e "s,^\(\[.*\):.*\],\1],"

    # hide base directories path
    sed $1 -i \
	-e "s,$SYSCONFDIR,SYSCONFDIR,g" \
	-e "s,$LOCALSTATEDIR,LOCALSTATEDIR,g" \
	-e "s,$DATAROOTDIR,DATAROOTDIR,g" \
	-e "s,$BINDIR,BINDIR,g" \
	-e "s,$EXEC_PREFIX,EXEC_PREFIX,g" \
	-e "s,$PREFIX,PREFIX,g"

    # hide user and group numbers
    sed $1 -i -e "s,^\(.*:x:\).*:,\1XXX:,"

    # hide group number
    sed $1 -i -e "s,:x:[[:digit:]]*:,:x:XXX:,"

    # hide cvs commit date and id
    sed $1 -i \
	-e "s,\(date:\) .*;  a,\1 XXX;  a," \
	-e "s,\(commitid:\) .*;,\1 XXX;,"
    
    # hide git commit date and id
    sed $1 -i \
	-e "s,^\(Date:  \).*,\1 XXX," \
	-e "s,^   [^ ]* \( master -> master\),   XXX..XXX \1," \
	-e "s,^\(commit\).*,\1 XXX," \
	-e "s,^\(\[master (commit racine) \)[^]]*,\1 XXX," \
	-e "s,^\(\[master \)[^]]*,\1 XXX," \
	
    # [master (commit racine) 1d93f9d]
    # [master 637f68c]
    # 1d93f9d..df4c07b  master -> master
    # commit df4c07b96124d2ddd73f6fd19edd7d42aa64e013
    # Date:   Sat Apr 23 23:47:39 2016 +0200
    
    # hide i386/amd64 defferences
    sed $1 -i -e "/^lib64$/ d"
}

# this function switch on into the unit test mode
# $1 the related module file name
# return 1 if not called by the module
function UNIT_TEST_start()
{
    [ -z $1 ] && echo "Please provide module file name"

    # this return when module is sourced
    [ $(basename "$0") = "ut$1.sh" ] || return 1
    UNIT_TEST_RUNNING=1

    # log to stderr for tests
    LOG_FACILITY=file
    LOG_SEVERITY=debug

    # root directory for tests requiering root user (jails...)
    if [ $(id -u) -eq 0 ]; then
	UNIT_TEST_ROOTDIR="/var/ut/$MDTX"
	UNIT_TEST_push_root_directory $UNIT_TEST_ROOTDIR
    fi

    # backup stdout and stderr values
    exec 3>&1 4>&2
    
    # redirect outputs to a file
    exec >scripts/$1.out 2>&1
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
    UNIT_TEST_EXPECTED="$srcdir/scripts/${1}.exp"
    [ -f $UNIT_TEST_EXPECTED ] || UNIT_TEST_EXPECTED="/dev/null"
    
    UNIT_TEST_mrProper scripts/$1.out
    diff $UNIT_TEST_EXPECTED scripts/$1.out

    # only needed for check_as_root
    if [ $(id -u) -eq 0 ]; then
	rm -fr /var/ut
    fi
}
