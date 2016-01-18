#!/bin/bash
#=======================================================================
# * Version: $Id: serv.sh,v 1.1 2015/07/01 10:49:26 nroche Exp $
# * Project: MediaTex
# * Module:  client modules (User API)
# *
# * Unit test script for serv.c
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

cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll2$SERVFILE
cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll3$SERVFILE
cp ${srcdir}/memory/user1Key_rsa.pub client/
cp ${srcdir}/memory/user3Key_dsa.pub client/

# run the unit test
client/ut$TEST -k >client/$TEST.out 2>&1 &
while [ -z "$(ps -ef | grep ut${TEST}[[:space:]])" ]; do :; done

# second run should fails
client/ut$TEST -k >client/$TEST.out2 2>&1 || /bin/true
killall -s SIGHUP ut$TEST 2>/dev/null || /bin/true
while [ "$(ps -ef | grep ut${TEST}[[:space:]])" ]; do :; done

# compare with the expected output
echo "=================================" >> client/$TEST.out
echo "  Second run " >> client/$TEST.out
echo "=================================" >> client/$TEST.out
cat client/$TEST.out2 >> client/$TEST.out
mrProperOutputs client/$TEST.out
sed -i client/$TEST.out -e "/kill/ d" 
diff $srcdir/client/$TEST.exp client/$TEST.out
