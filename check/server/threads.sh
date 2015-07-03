#!/bin/bash
#=======================================================================
# * Version: $Id: threads.sh,v 1.2 2015/07/03 11:39:01 nroche Exp $
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

# retrieve environment
[ -z $srcdir ] && srcdir=.
. utmediatex.sh

TEST=$(basename $0)
TEST=${TEST%.sh}

# run the unit test
rm -f $PIDFILE
server/ut$TEST >server/$TEST.out 2>&1 &
PID=$!
echo $PID > $PIDFILE

# wait until ready
while [ -z "$(ps -ef | grep [u]t$TEST)" ]; do :; done

# 1 socket + 1 USR1 signal
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
common/utregister -W 2>/dev/null

# HUP wait end of jobs
kill -s HUP $PID

# 3 sockets
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true

# synchronize with daeomon
shm=$(common/utregister -G 2>/dev/null);
while [ "$shm" != "=> 0000" ]; do
	shm=$(common/utregister -G 2>/dev/null)
done

# 3 USR1 signals
common/utregister -W 2>/dev/null
common/utregister -W 2>/dev/null
common/utregister -W 2>/dev/null

# 1 more socket and 1 more USR1 signals
telnet 127.0.0.1 6560 >/dev/null 2>&1 || true
common/utregister -W 2>/dev/null

# TERM wait end of started jobs (socket)
kill -s TERM $PID

# waiting for the end of the daemon
while [ "$(ps -ef | grep [u]t$TEST)" ]; do :; done

# compare with the expected output
mrProperOutputs server/$TEST.out
sed server/$TEST.out -i \
    -e 's/Daemon (.*)/Daemon (XXXX)/' \
    -e 's/from 127\.0\.0\.1:.*$/from 127.0.0.1:XXXXX (localhost)/' \
    -e 's/localhost.localdomain/localhost/'

diff $srcdir/server/$TEST.exp server/$TEST.out

# sort to compare with or without multi-cpu
#sort $srcdir/server/threads.exp > server/$TEST.sexp
#sort server/$TEST.out > server/$TEST.sort
#diff server/$TEST.sexp server/$TEST.sort

