#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * unit test for git manager
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
#set -x
set -e

if [ $(id -u) -ne 0 ]; then
    echo "(root needed for this test)"
    exit 77 # SKIP
fi

MDTX_MDTXUSER="ut4-mdtx"

# includes
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh
[ ! -z $MDTX_SH_SSH ] || source $libdir/ssh.sh
[ ! -z $MDTX_SH_GIT ] || source $libdir/git.sh
source $srcdir/../check/utscripts.sh

UNIT_TEST_start "git"
Debug "test for git.sh"

COLL="hello"
USER="$MDTX-$COLL"

# cleanup if previous test has failed
USERS_coll_remove_user $USER
USERS_mdtx_remove_user
rm -fr $GITBARE

# cf init.sh
USERS_root_populate
USERS_mdtx_create_user
GIT_mdtx_import
    
# test mdtx module
echo -e "\n# test mdtx pull\n" >> $GITCLT/$MDTX/$MDTX.conf
GIT_commit $MDTX "Manual user edition"
GIT_pull $MDTX
echo -e "\n# test mdtx push\n" >> $GITCLT/$MDTX/$MDTX.conf
GIT_commit $MDTX "Unit test"
GIT_push $MDTX
cd $GITCLT/$MDTX
su $MDTX -c "git log $MDTX.conf"
cd - >/dev/null
    
# cf new.sh
USERS_coll_create_user $USER
SSH_build_key $USER
GIT_coll_import $USER
SSH_bootstrapKeys $USER
SSH_configure_client $USER "localhost" 22

# test ssh
QUERY="ssh -o PasswordAuthentication=no ${USER}@localhost ls"
Info "su $USER -c \"$QUERY\""
su $USER -c "$QUERY" || Error "Cannot connect via ssh"

# emulate collection checkout (as not using a jail)
QUERY="git clone ssh://$USER@localhost:$GITBARE/$USER $GITCLT/$USER"
rm -fr $GITCLT/$USER
USERS_install $GIT "${_VAR_LIB_M_MDTX_COLL[@]}"
Info "su $USER -c \"$QUERY\""
su $USER -c "$QUERY" || Error "Cannot checkout via ssh"
GIT_upgrade $USER "HOSTNAME" "FINGERPRINT"

# test collection module
echo -e "\n# test coll pull\n" >> $GITCLT/$USER/catalog000.txt
GIT_commit $USER "Manual user edition"
GIT_pull $USER
echo -e "\n# test coll push\n" >> $GITCLT/$USER/catalog000.txt
GIT_commit $USER "coll unit test"
GIT_push $USER
cd $GITCLT/$USER
su $MDTX -c "git log catalog000.txt"
cd - >/dev/null
   
# cf free.sh & remove.sh
USERS_coll_remove_user $USER
USERS_mdtx_remove_user
rm -fr $GITBARE

Info "success" 
UNIT_TEST_stop "git"
