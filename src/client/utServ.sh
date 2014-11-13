#!/bin/bash
#=======================================================================
# * Version: $Id: utServ.sh,v 1.2 2014/11/13 16:36:22 nroche Exp $
# * Project: MediaTex
# * Module:  client modules (User API)
# *
# * Unit test script for serv.c
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
cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll2$SERVFILE
cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll3$SERVFILE
cp ${srcdir}/../memory/user1Key_rsa.pub .
cp ${srcdir}/../memory/user3Key_dsa.pub .

./utserv -k >ut.out 2>&1 &
sleep 1

# second run should fails
./utserv -k >ut.out2 2>&1 || /bin/true
killall -s SIGHUP utserv 2>/dev/null || /bin/true
sleep 1

# compare with the expected output
echo "=================================" >> ut.out
echo "  Second run " >> ut.out
echo "=================================" >> ut.out
cat ut.out2 >> ut.out
mrProperOutputs
sed -i -e "/kill/ d" ut.out
diff $srcdir/utServ.exp ut.out
