#!/bin/bash
#set -x
set -e
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * This script uninstall the MediaTex software
# * ... to be copy past into debian/postrm for purge
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

[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/lib
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ]   || source $libdir/users.sh
[ ! -z $MDTX_SH_SSH ]     || source $libdir/ssh.sh
[ ! -z $MDTX_SH_JAIL ]    || source $libdir/jail.sh
[ ! -z $MDTX_SH_HTDOCS ]  || source $libdir/htdocs.sh

Debug "purge"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"

# only the init script for mdtx server is manage here
if [ $MDTX = mdtx ]; then
    invoke-rc.d ${MEDIATEX#/}d stop $MDTX
    update-rc.d -f ${MEDIATEX#/}d remove
fi

JAIL_unbind
SSH_chroot_login no

USERS=$(grep "^$MDTX-" /etc/passwd | cut -d':' -f1) || true
for _USER in $USERS; do
    USERS_coll_remove_user $_USER
done

USERS_mdtx_remove_user
USERS_root_disease

if [ -f /etc/apache2/conf-enabled/${MEDIATEX#/}-$MDTX.conf ]; then
    /usr/sbin/a2disconf ${MEDIATEX#/}-$MDTX.conf
fi
HTDOCS_unconfigure_mdtx_apache2

Info "Please check if these apache modules we were using are still needed:"
Info "* auth_digest autoindex env include rewrite userdir ssl"
Info "done"

/usr/sbin/invoke-rc.d apache2 restart
