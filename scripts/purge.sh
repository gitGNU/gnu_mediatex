#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: purge.sh,v 1.1 2014/10/13 19:38:35 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This script uninstall the MediaTex software
# * ... to be copy past into debian/postrm for purge
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
[ ! -z $MDTX_SH_SSH ]     || source $libdir/ssh.sh
[ ! -z $MDTX_SH_JAIL ]    || source $libdir/jail.sh

Debug "purge"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"

invoke-rc.d ${MEDIATEX#/}d stop $MDTX
update-rc.d -f ${MEDIATEX#/}d remove

JAIL_unbind
SSH_chroot_login no

USERS=$(grep "^$MDTX-" /etc/passwd | cut -d':' -f1) || true
for _USER in $USERS; do
    USERS_coll_remove_user $_USER
done

USERS_mdtx_remove_user
USERS_root_disease

Info "Please check if these apache modules we were using are still needed:"
Info "* auth_digest autoindex env include rewrite userdir ssl"
Info "done"