#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: addUser.sh,v 1.2 2014/11/13 16:36:09 nroche Exp $
# * Project: MediaTex
# * Module : scripts
# *
# * This script add a user to mdtx's groups
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

[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/lib
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ]   || source $libdir/users.sh

Debug "addUser"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"
[ ! -z $1 ] || Error "expect a user as first parameter"
USER=$1

USERS_add_to_group $USER $MDTX
USERS_add_to_group $USER ${MDTX}_md

_GROUPS=$(grep "^$MDTX-" /etc/group | cut -d':' -f1)
for GROUP in $_GROUPS; do
    USERS_add_to_group $USER $GROUP
done

Info "done"