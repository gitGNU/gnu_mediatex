#!/bin/bash
#set -x
set -e
#=======================================================================
# * Project: MediaTex
# * Module : scripts
# *
# * This script setup the MediaTex software
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
[ ! -z $MDTX_SH_GIT ]     || source $libdir/git.sh
[ ! -z $MDTX_SH_JAIL ]    || source $libdir/jail.sh
[ ! -z $MDTX_SH_HTDOCS ]  || source $libdir/htdocs.sh

Debug "init"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"

USERS_root_populate
USERS_mdtx_create_user
GIT_mdtx_import
SSH_chroot_login yes
JAIL_unbind
JAIL_build $MDTX
HTDOCS_configure_mdtx_cgit
HTDOCS_configure_mdtx_apache2

# only needed once
/usr/sbin/a2enmod \
    authz_core auth_digest authz_groupfile autoindex cgi env include \
    rewrite userdir setenvif ssl
/usr/sbin/a2ensite default-ssl
/usr/sbin/a2enconf mediatex
/usr/sbin/invoke-rc.d rsyslog restart
/sbin/ldconfig

# mdtx config for apache
/usr/sbin/a2enconf ${MEDIATEX#/}-$MDTX.conf >/dev/null
/usr/sbin/invoke-rc.d apache2 restart

# script is not automatically installed for other instances than mdtx
if [ $MDTX = mdtx ]; then
    /usr/sbin/update-rc.d ${MEDIATEX#/}d defaults
    /usr/sbin/invoke-rc.d ${MEDIATEX#/}d restart $MDTX
fi

JAIL_bind # not needed but more confortable
Info "done"

