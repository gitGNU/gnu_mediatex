#!/bin/bash
#=======================================================================
# * Version: $Id: utHave.sh,v 1.1 2014/10/13 19:39:59 nroche Exp $
# * Project: MediaTex
# * Module:  server modules
# *
# * Unit test script for have.c
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
#set -x
set -e

[ -z $srcdir ] && srcdir=.

# retrieve environment
. ${srcdir}/../utMediatex.sh

# run the unit test
cp ${srcdir}/../../examples/logoP1.cat /tmp
cp ${srcdir}/../../examples/logoP2.cat /tmp
./uthave -d ${srcdir} >ut.out 2>&1

# compare with the expected output
mrProperOutputs
diff $srcdir/utHave.exp ut.out 
