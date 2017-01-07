#!/bin/bash
#=======================================================================
# * Project: Jukebox
# * Module: tests
# *
# * Unit test script for aggregation
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014 2015 2016 2017 Nicolas Roche
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

# create files to aggregate
rm -fr tmp/test4
mkdir -p tmp/test4
touch tmp/test4/gz.txt
gzip tmp/test4/gz.txt

for S in 2 3 5 7 11; do
    dd bs=1048576 count=$S if=/dev/zero of=tmp/test4/zero$S.bin
    cat tmp/test4/gz.txt.gz tmp/test4/zero$S.bin > tmp/test4/zero$S.gz
    rm tmp/test4/zero$S.bin
done
rm tmp/test4/gz.txt.gz

$srcdir/test.sh test4

