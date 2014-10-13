#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: module.sh,v 1.1 2014/10/13 19:38:41 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This file is a bare-bones for mediatex script library
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

# includes
MDTX_SH_MODULE=1
[ -z $srcdir ] && srcdir=..
[ -z $libdir ] && libdir=$srcdir
[ ! -z $MDTX_SH_UNIT_TEST ] || source $libdir/unitTest.sh
[ ! -z $MDTX_SH_LOG ]       || source $libdir/log.sh

function fonction()
{
    Debug $FUNCNAME 2
    Info "$1"
}

# unitary tests
if UNIT_TEST_start "module"; then

    fonction "unit test"
    Info "success"

    UNIT_TEST_stop "module"
fi

