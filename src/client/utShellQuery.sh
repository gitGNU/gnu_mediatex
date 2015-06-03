#!/bin/bash
#=======================================================================
# * Version: $Id: utShellQuery.sh,v 1.3 2015/06/03 14:03:32 nroche Exp $
# * Project: MediaTex
# * Module:  client modules (User API)
# *
# * Unit test script for shellquery.c
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
. ${srcdir}/../utMediatex.sh

# run the unit tests
rm -f ut.out

# admConfQuery
./utshellQuery.tab -P adm init >>ut.out 2>&1
./utshellQuery.tab -P adm remove >>ut.out 2>&1
./utshellQuery.tab -P adm purge >>ut.out 2>&1

./utshellQuery.tab -P adm add user USER >>ut.out 2>&1
./utshellQuery.tab -P adm del user USER >>ut.out 2>&1
./utshellQuery.tab -P adm add coll COLL >>ut.out 2>&1
./utshellQuery.tab -P adm add coll COLL@HOST >>ut.out 2>&1
./utshellQuery.tab -P adm add coll COLL@HOST:33 >>ut.out 2>&1
./utshellQuery.tab -P adm add coll COLL:33 >>ut.out 2>&1
./utshellQuery.tab -P adm del coll COLL >>ut.out 2>&1

./utshellQuery.tab -P adm update >>ut.out 2>&1
./utshellQuery.tab -P adm update coll COLL >>ut.out 2>&1
./utshellQuery.tab -P adm commit >>ut.out 2>&1
./utshellQuery.tab -P adm commit coll COLL >>ut.out 2>&1
./utshellQuery.tab -P adm bind >>ut.out 2>&1
./utshellQuery.tab -P adm unbind >>ut.out 2>&1
./utshellQuery.tab -P adm mount ISO on PATH >>ut.out 2>&1
./utshellQuery.tab -P adm umount PATH >>ut.out 2>&1
./utshellQuery.tab -P adm get PATH1 as COLL on PATH2 >>ut.out 2>&1

# srvQuery
./utshellQuery.tab -P srv save >>ut.out 2>&1
./utshellQuery.tab -P srv extract >>ut.out 2>&1
./utshellQuery.tab -P srv notify >>ut.out 2>&1
./utshellQuery.tab -P srv deliver >>ut.out 2>&1

# apiSuppQuery
./utshellQuery.tab -P add supp SUPP on PATH >>ut.out 2>&1
./utshellQuery.tab -P del supp SUPP >>ut.out 2>&1
./utshellQuery.tab -P list supp >>ut.out 2>&1
./utshellQuery.tab -P add supp SUPP to ALL >>ut.out 2>&1
./utshellQuery.tab -P add supp SUPP to coll COLL >>ut.out 2>&1
./utshellQuery.tab -P del supp SUPP from ALL >>ut.out 2>&1
./utshellQuery.tab -P del supp SUPP from coll COLL >>ut.out 2>&1
./utshellQuery.tab -P note supp SUPP as TEXT >>ut.out 2>&1
./utshellQuery.tab -P check supp SUPP on PATH >>ut.out 2>&1
./utshellQuery.tab -P upload PATH to coll COLL >>ut.out 2>&1

# apiCollQuery
./utshellQuery.tab -P add key keyFile.txt to coll COLL >>ut.out 2>&1
./utshellQuery.tab -P del key 0123456789abcdef0123456789abcdef from coll COLL >>ut.out 2>&1
./utshellQuery.tab -P list coll >>ut.out 2>&1
./utshellQuery.tab -P motd >>ut.out 2>&1
./utshellQuery.tab -P upgrade >>ut.out 2>&1
./utshellQuery.tab -P upgrade coll COLL >>ut.out 2>&1
./utshellQuery.tab -P make >>ut.out 2>&1
./utshellQuery.tab -P make coll COLL >>ut.out 2>&1
./utshellQuery.tab -P clean >>ut.out 2>&1
./utshellQuery.tab -P clean coll COLL >>ut.out 2>&1
./utshellQuery.tab -P su >>ut.out 2>&1
./utshellQuery.tab -P su coll COLL >>ut.out 2>&1

# compare with the expected output
mrProperOutputs
diff $srcdir/utShellQuery.exp ut.out 

