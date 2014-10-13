#!/bin/bash
#=======================================================================
# * Version: $Id: utConnect.sh,v 1.1 2014/10/13 19:39:01 nroche Exp $
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for connect.c
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

# retrieve environment
[ -z $srcdir ] && srcdir=.
. ${srcdir}/../utMediatex.sh

# run the unit tests
echo "*** no server:" > ut.out
./utconnect >> ut.out 2>&1 || true

echo "*** with server:" >> ut.out
nc -l -p 12345 > ut.txt & 
PID=$!
sleep 1
./utconnect >> ut.out 2>&1
echo "*** server receive:" >> ut.out
cat ut.txt >> ut.out
rm ut.txt

# compare with the expected output
mrProperOutputs
diff $srcdir/utConnect.exp ut.out
