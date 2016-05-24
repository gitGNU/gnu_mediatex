#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for register.c
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

# retrieve environment
[ -z $srcdir ] && srcdir=.
. utmediatex.sh

TEST=$(basename $0)
TEST=${TEST%.sh}

# run the unit tests

# when ourself receive SIGUSR1, we display the shm and re-init it
echo $$ > $PIDFILE
trap "common/ut$TEST -G -swarning >>common/$TEST.out 2>&1 && 
common/ut$TEST -I -swarning >>common/$TEST.out 2>&1" SIGUSR1

echo "* Initialize shm" >common/$TEST.out
common/ut$TEST -I >>common/$TEST.out 2>&1
common/ut$TEST -G -swarning >>common/$TEST.out 2>&1

# we send the signal, wait it is received and finaly display the shm
echo "* Send save message" >>common/$TEST.out
common/ut$TEST -W >>common/$TEST.out 2>&1 &
wait || common/ut$TEST -G -swarning >>common/$TEST.out 2>&1

echo "* Send extract message" >>common/$TEST.out
common/ut$TEST -E >>common/$TEST.out 2>&1 &
wait || common/ut$TEST -G -swarning >>common/$TEST.out 2>&1

echo "* Send notify message" >>common/$TEST.out
common/ut$TEST -N >>common/$TEST.out 2>&1 &
wait || common/ut$TEST -G -swarning >>common/$TEST.out 2>&1

echo "* Send deliver message" >>common/$TEST.out
common/ut$TEST -D >>common/$TEST.out 2>&1 &
wait || common/ut$TEST -G -swarning >>common/$TEST.out 2>&1

echo "* Free shm" >>common/$TEST.out
#sleep 1
common/ut$TEST -F >>common/$TEST.out 2>&1
rm -f $PIDFILE

# compare with the expected output
sed common/$TEST.out -i \
    -e 's/to .*$/to XXXX/' \
    -e '/exit on success/ d'
mrProperOutputs common/$TEST.out
diff $srcdir/common/$TEST.exp common/$TEST.out
