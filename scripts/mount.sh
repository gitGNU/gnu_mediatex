#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: mount.sh,v 1.5 2015/08/23 23:39:15 nroche Exp $
# * Project: MediaTex
# * Module : mount script
# *
# * This script mount an iso device or file
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

Debug "mount"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$1" ] || Error "please provide a path to an ISO"
[ ! -z "$2" ] || Error "please provide a relative path where to mount"
ISO=$1
MNT=$2
OPTIONS="-o ro"$3

/bin/mount -t iso9660 $OPTIONS $ISO $MNT
Info "done"