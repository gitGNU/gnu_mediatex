#!/bin/bash
#=======================================================================
# * Version: $Id: locks.sh,v 1.2 2015/08/23 23:39:11 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for locks.c
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

# run units tests

# 1) A single reader
misc/ut$TEST -i /dev/null -p R >misc/$TEST.out1 2>&1 &
PID1=$!

# 2) Two readers may read in the same time
misc/ut$TEST -i /dev/null -p R >misc/$TEST.out2 2>&1 &
PID2=$!

# 3) writer cannot erase the file while other are reading
while [ -z "$(ps -ef | grep [u]t$TEST)" ]; do :; done

misc/ut$TEST -i /dev/null -p W >misc/$TEST.out3 2>&1 &

# 4) A single writer
while [ ! -z "$(ps -ef | grep [u]t$TEST | grep W)" ]; do :; done
kill -SIGUSR1 $PID1
kill -SIGUSR1 $PID2
while [ ! -z "$(ps -ef | grep [u]t$TEST)" ]; do :; done
misc/ut$TEST -i /dev/null -p W >misc/$TEST.out4 2>&1 &

# 5) Two writers (or -6- writer and readers) cannot run in the same time
while [ -z "$(ps -ef | grep [u]t$TEST)" ]; do :; done
misc/ut$TEST -i /dev/null -p W >misc/$TEST.out5 2>&1 &
misc/ut$TEST -i /dev/null -p R >misc/$TEST.out6 2>&1 &

while [ -z "$(ps -ef | grep [u]t$TEST | grep -- -p)" ]; do :; done
killall ut$TEST 2>/dev/null || /bin/true
while [ ! -z "$(ps -ef | grep [u]t$TEST)" ]; do :; done
rm -f misc/$TEST.out
for i in $(seq 1 6)
do
    echo "=========== $i ============" >> misc/$TEST.out
    cat misc/$TEST.out${i} >> misc/$TEST.out
    rm -f misc/$TEST.out${i}
done

# compare with the expected output
mrProperOutputs misc/$TEST.out
sed -i -e "/kill/ d" -e "/malloc/ d" misc/$TEST.out
diff $srcdir/misc/$TEST.exp misc/$TEST.out

