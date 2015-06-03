#!/bin/bash
#=======================================================================
# * Version: $Id: utRecordList.sh,v 1.3 2015/06/03 14:03:54 nroche Exp $
# * Project: MediaTex
# * Module:  parser modules
# *
# * Unit test script for recordlist.c
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

# run the unit test without encryption
IN=${MD5SUMS}/${MDTXUSER}-coll1.md5
./utrecordList.tab -i $IN -o ut.out >utRecordList.txt 2>&1

# compare with the expected output
diff $IN ut.out

# run the unit test with encryption
IN=${MD5SUMS}/${MDTXUSER}-coll1.aes
./utrecordList.tab -i $IN -o utAes.out >utRecordList.txt 2>&1

# compare with the expected output
diff $IN utAes.out

