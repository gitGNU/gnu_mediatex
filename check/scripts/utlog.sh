#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * Provide log facilities
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
#set -x
set -e

# includes
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ]  || source $libdir/log.sh
source $srcdir/utscripts.sh

UNIT_TEST_start "log"

# $1: indirection level
function indirection1()
{
    Log "info" "indirection level=$[$1+1]" $[$1+1]
}

# $1: indirection level
function indirection2()
{
    indirection1 $[$1+1]
}

# $1: indirection level
function indirection3()
{
    indirection2 $[$1+1]
}

# log to stdout
Log "info" "indirection level=0" 0
indirection1 0
indirection2 0
indirection3 0

# only_root indirection3 0
Warning "warning message"
Info "info message"
Debug "debug message"

# log to syslog
LOG_FACILITY="local2"
Warning "warning message for syslog"
Info "info message for syslog"
Debug "debug message for syslog"

UNIT_TEST_stop "log"
