#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * unit test for jail manager
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

MDTX_MDTXUSER="ut5-mdtx"

# includes
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh
[ ! -z $MDTX_SH_SSH ] || source $libdir/ssh.sh
[ ! -z $MDTX_SH_GIT ] || source $libdir/git.sh
[ ! -z $MDTX_SH_JAIL ] || source $libdir/jail.sh
source $srcdir/../check/utscripts.sh

UNIT_TEST_start "jail"
Debug "test for jail.sh"

COLL="hello"
USER="$MDTX-$COLL"

# cleanup if previous test has failed
JAIL_unbind
USERS_mdtx_remove_user
USERS_coll_remove_user $USER
USERS_mdtx_disease
rm -fr $GITBARE

# cf init.sh
USERS_root_populate
USERS_mdtx_create_user
GIT_mdtx_import
SSH_chroot_login yes
JAIL_build

## local check
Info "chroot $JAIL ls"
chroot $JAIL ls

# cf new.sh
USERS_coll_create_user $USER
SSH_build_key $USER
GIT_coll_import $USER
SSH_bootstrapKeys $USER
SSH_configure_client $USER "localhost" "22"
JAIL_add_user $USER	

## remote login checks
QUERY="ssh -o PasswordAuthentication=no ${USER}@localhost ls /"
Info "su LABEL -c \"$QUERY\""
su $USER -c "$QUERY" || Error "Cannot connect via ssh"

## cf init.d 
JAIL_bind
    
## scp and git checks
touch $CACHEDIR/$MDTX/cache/$USER/hello.txt
chown $USER.$USER $CACHEDIR/$MDTX/cache/$USER/hello.txt

CACHE="$CACHEDIR/$MDTX/tmp/$USER"
QUERY="scp ${USER}@localhost:/var/cache/$USER/hello.txt $CACHE"

Info "su USER -c \"$QUERY\""
su $USER -c "$QUERY" || Error "Cannot copy via ssh"
GIT_coll_checkout $USER $MDTX $COLL "localhost"
    
#echo "results :"
#find $UNIT_TEST_ROOTDIR -ls |
#awk '{ printf("%14s %14s %s %s\n",$5,$6,$3,$11) }' |
#sort -k4

# cf init.d
JAIL_unbind
JAIL_del_user $USER

# cf free.sh & remove.sh
SSH_chroot_login no
USERS_coll_disease $USER
USERS_coll_remove_user $USER
USERS_mdtx_remove_user
rm -fr $GITBARE

Info "success"
UNIT_TEST_stop "jail"
