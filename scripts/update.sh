#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: update.sh,v 1.4 2015/06/30 17:37:22 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This script update a MediaTex module
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
[ ! -z $MDTX_SH_INCLUDE ]  || source $libdir/include.sh
[ ! -z $MDTX_SH_CVS ]      || source $libdir/cvs.sh

Debug "update"

MODULE=$1
[ -z $1 ] && Error "please provide a module"
[ "$(whoami)" == "$MODULE" ] || Error "need to be $MODULE user"

CVS_update $MODULE ||
    Error $0 "cannot update $MODULE cvs directory"

Info "done"
