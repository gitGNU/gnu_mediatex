#!/bin/bash
#=======================================================================
# * Version: $Id: utThreads.sh,v 1.3 2015/06/03 14:03:58 nroche Exp $
# * Project: MediaTex
# * Module:  server modules
# *
# * Unit test script for threads.c
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
#set -x
set -e

[ -z $srcdir ] && srcdir=.

# retrieve environment
. ${srcdir}/../utMediatex.sh

#cp ${HMEDIR}/test1/cvs/servers.txt ${HMEDIR}/test3/cvs/servers.txt

# run the unit test
rm -f $PIDFILE
./utthreads >ut.out 2>&1 &
PID=$!
echo $PID > $PIDFILE
sleep 1

# 1 socket + 1 USR1 signal
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
../common/utregister -W 2>/dev/null

# HUP wait end of jobs
kill -s HUP $PID

# 3 sockets
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true

# synchronize with daeomon
shm=$(../common/utregister -G 2>/dev/null);
while [ "$shm" != "=> 0000" ];
do
	shm=$(../common/utregister -G 2>/dev/null);
	sleep 1;
done

# 3 USR1 signals
../common/utregister -W 2>/dev/null
../common/utregister -W 2>/dev/null
../common/utregister -W 2>/dev/null

# 1 more socket and 1 more USR1 signals
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
../common/utregister -W 2>/dev/null

# TERM wait end of started jobs (socket)
kill -s TERM $PID

# synchronize makefile with daeomon
while [ -n "$(../utregister -G 2>/dev/null)" ];
do
	sleep 1;
done

# compare with the expected output
mrProperOutputs
sed -i -e 's/Daemon (.*)/Daemon (XXXX)/' ut.out
sed -i -e 's/from 127\.0\.0\.1:.*$/from 127.0.0.1:XXXXX (localhost)/' ut.out

# sort to compare with or without multi-cpu
sort $srcdir/utThreads.exp > utThreads.sort
sort ut.out | sort > ut.sort
diff utThreads.sort ut.sort
