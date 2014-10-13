#!/bin/bash
#=======================================================================
# * Version: $Id: utRegister.sh,v 1.1 2014/10/13 19:39:03 nroche Exp $
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for register.c
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
#set -e

# retrieve environment
[ -z $srcdir ] && srcdir=.
. ${srcdir}/../utMediatex.sh

# run the unit tests

# when ourself receive SIGUSR1, we display the shm and re-init it
echo $$ > $PIDFILE
trap "./utregister -G >>ut.out 2>/dev/null && 
./utregister -I >>ut.out 2>/dev/null" SIGUSR1

echo "* Initialize shm" >ut.out
./utregister -I >>ut.out 2>&1
./utregister -G >>ut.out 2>/dev/null

# we send the signal, wait it is received and finaly display the shm
echo "* Send save message" >>ut.out
./utregister -S >>ut.out 2>&1 &
wait || ./utregister -G >>ut.out 2>/dev/null

echo "* Send extract message" >>ut.out
./utregister -E >>ut.out 2>&1 &
wait || ./utregister -G >>ut.out 2>/dev/null

echo "* Send notify message" >>ut.out
./utregister -N >>ut.out 2>&1 &
wait || ./utregister -G >>ut.out 2>/dev/null

echo "* Send deliver message" >>ut.out
./utregister -D >>ut.out 2>&1 &
wait || ./utregister -G >>ut.out 2>/dev/null

echo "* Free shm" >>ut.out
sleep 1
./utregister -F >>ut.out 2>&1

# compare with the expected output
sed -i -e 's/to .*$/to XXXX/' ut.out
mrProperOutputs

# sort to compare with or without multi-cpu
sort $srcdir/utRegister.exp > utRegister.sort
sort ut.out | sort > ut.sort
diff utRegister.sort ut.sort
