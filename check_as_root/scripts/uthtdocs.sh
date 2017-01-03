#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * unit test for apache and viewvc manager
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014 2015 2016 2017 Nicolas Roche
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
#set -x
set -e

if [ $(id -u) -ne 0 ]; then
    echo "(root needed for this test)"
    exit 77 # SKIP
fi

MDTX_MDTXUSER="ut2-mdtx"

# includes
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh
[ ! -z $MDTX_SH_HTDOCS ] || source $libdir/htdocs.sh
source $srcdir/../check/utscripts.sh

UNIT_TEST_start "htdocs"
Debug "test for htdocs.sh"

COLL="hello"
USER="$MDTX-$COLL"

# cleanup if previous test has failed
USERS_mdtx_remove_user
USERS_coll_remove_user $USER

# init.sh
USERS_root_populate
USERS_mdtx_create_user
HTDOCS_configure_mdtx_apache2
HTDOCS_configure_mdtx_cgit

# new.sh
USERS_coll_create_user $USER
HTDOCS_configure_coll_apache2 $USER

echo "* files : *"
find $UNIT_TEST_ROOTDIR -ls |
awk '{ printf("%14s %14s %s %s\n",$5,$6,$3,$11) }' |
sort -k4

# free.sh/purge.sh
HTDOCS_unconfigure_mdtx_apache2
USERS_coll_remove_user $USER
USERS_mdtx_remove_user

echo "* cleanup : *"
find $UNIT_TEST_ROOTDIR -ls |
awk '{ printf("%s %s\n",$3,$11) }' |
sort -k4

Info "success"
UNIT_TEST_stop "htdocs"
