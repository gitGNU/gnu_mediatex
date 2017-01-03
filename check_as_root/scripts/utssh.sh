#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * unit test for ssh manager
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

MDTX_MDTXUSER="ut3-mdtx"

# includes
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh
[ ! -z $MDTX_SH_SSH ] || source $libdir/ssh.sh
source $srcdir/../check/utscripts.sh

UNIT_TEST_start "ssh"
Debug "test for ssh.sh"

COLL="hello"
USER="$MDTX-$COLL"


# cleanup if previous test has failed
USERS_mdtx_remove_user
USERS_coll_remove_user $USER
USERS_mdtx_disease

# cf init.sh
USERS_root_populate
USERS_mdtx_create_user

# cf new.sh
USERS_coll_create_user $USER
SSH_build_key $USER
SSH_bootstrapKeys $USER
SSH_configure_client $USER "localhost" 22

## check ssh connection
QUERY="ssh -o PasswordAuthentication=no ${USER}@localhost ls"
Info "su USER -c \"$QUERY\""
su $USER -c "$QUERY" || Error "Cannot connect via ssh"
    
# cf free.sh
USERS_coll_disease $USER
USERS_coll_remove_user $USER
USERS_mdtx_remove_user

Info "success"
UNIT_TEST_stop "ssh"
