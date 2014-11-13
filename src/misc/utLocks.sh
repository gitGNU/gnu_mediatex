#!/bin/bash
#=======================================================================
# * Version: $Id: utLocks.sh,v 1.2 2014/11/13 16:36:53 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for locks.c
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

# retrieve environment
[ -z $srcdir ] && srcdir=.
. ${srcdir}/../utMediatex.sh

# run units tests

# 1) A single reader
./utlocks -i /dev/null -p R >ut.out1 2>&1 &
PID1=$!

# 2) Two readers may read in the same time
./utlocks -i /dev/null -p R >ut.out2 2>&1 &
PID2=$!

# 3) writer cannot erase the file while other are reading
sleep 1 # else writer may keep the lock
./utlocks -i /dev/null -p W >ut.out3 2>&1 &

# 4) A single writer
sleep 1 # else previous writer may keep the lock
kill -SIGUSR1 $PID1
kill -SIGUSR1 $PID2
sleep 1 # else lock is not free
./utlocks -i /dev/null -p W >ut.out4 2>&1 &

# 5) Two writers run in the same time
sleep 1 # else first writer may keep the lock
./utlocks -i /dev/null -p R >ut.out5 2>&1 &

killall utlocks 2>/dev/null || /bin/true
sleep 1 # else ut.out4 is not full
rm -f ut.out
for i in $(seq 1 5)
do
    echo "=========== $i ============" >> ut.out
    cat ut.out${i} >> ut.out
    rm -f ut.out${i}
done

# compare with the expected output
mrProperOutputs
sed -i -e "/kill/ d" ut.out
diff $srcdir/utLocks.exp ut.out

