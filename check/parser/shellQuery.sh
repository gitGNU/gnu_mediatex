#!/bin/bash
#=======================================================================
# * Version: $Id: shellQuery.sh,v 1.1 2015/07/01 10:49:59 nroche Exp $
# * Project: MediaTex
# * Module:  shellQuery
# *
# * Unit test script for shellQuery.y
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

# run the unit tests
rm -f parser/$TEST.out

# admConfQuery
parser/ut$TEST.tab -P adm init >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm remove >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm purge >>parser/$TEST.out 2>&1

parser/ut$TEST.tab -P adm add user USER >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm del user USER >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm add coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm add coll COLL@HOST >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm add coll COLL@HOST:33 >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm add coll COLL:33 >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm del coll COLL >>parser/$TEST.out 2>&1

parser/ut$TEST.tab -P adm update >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm update coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm commit >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm commit coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm bind >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm unbind >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm mount ISO on PATH >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm umount PATH >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P adm get PATH1 as COLL on PATH2 >>parser/$TEST.out 2>&1

# srvQuery
parser/ut$TEST.tab -P srv save >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P srv extract >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P srv notify >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P srv deliver >>parser/$TEST.out 2>&1

# apiSuppQuery
parser/ut$TEST.tab -P add supp SUPP on PATH >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P del supp SUPP >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P list supp >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P add supp SUPP to ALL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P add supp SUPP to coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P del supp SUPP from ALL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P del supp SUPP from coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P note supp SUPP as TEXT >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P check supp SUPP on PATH >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P upload PATH to coll COLL >>parser/$TEST.out 2>&1

# apiCollQuery
parser/ut$TEST.tab -P add key keyFile.txt to coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P del key 0123456789abcdef0123456789abcdef from coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P list coll >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P motd >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P upgrade >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P upgrade coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P make >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P make coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P clean >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P clean coll COLL >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P su >>parser/$TEST.out 2>&1
parser/ut$TEST.tab -P su coll COLL >>parser/$TEST.out 2>&1

# compare with the expected output
mrProperOutputs parser/$TEST.out
diff $srcdir/parser/$TEST.exp parser/$TEST.out

