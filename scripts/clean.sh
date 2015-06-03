#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: clean.sh,v 1.3 2015/06/03 14:03:23 nroche Exp $
# * Project: MediaTex
# * Module : script clean
# *
# * This script clean a MediaTex HTML catalog
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014 2015 Nicolas Roche
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
[ ! -z $MDTX_SH_INCLUDE ]  || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ]    || source $libdir/users.sh
[ ! -z $MDTX_SH_HTDOCS ]   || source $libdir/htdocs.sh

Debug "clean"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z $1 ] || Error "expect a label as first parameter"
[ ! "$1" = "mdtx" ] || Error "collection cannot be labeled mdtx"
USER="$MDTX-$1"

rm -fr $CACHEDIR/$MDTX/home/$USER/public_html
USERS_coll_populate $USER
HTDOCS_configure_coll_apache2 $USER
HTDOCS_configure_coll_viewvc $USER

Info "done"