#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: remove.sh,v 1.1 2014/10/13 19:38:36 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This script uninstall the MediaTex software
# * ... to be call by debian/prerm
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

Debug "remove"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"

invoke-rc.d ${MEDIATEX#/}d stop $MDTX

JAIL_unbind
SSH_chroot_login no

Info "done"