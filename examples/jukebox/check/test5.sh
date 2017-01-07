#!/bin/bash
#=======================================================================
# * Project: Jukebox
# * Module: tests
# *
# * Unit test script for aggregation having toot much iso metatdata
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
rm -fr tmp/test5
mkdir -p tmp/test5
touch tmp/test5/gz.txt
gzip tmp/test5/gz.txt

for S in 2 3 5 7 11; do
    dd bs=1048576 count=$S if=/dev/zero of=tmp/test5/zero$S.bin
    cat tmp/test5/gz.txt.gz tmp/test5/zero$S.bin > tmp/test5/zero$S.gz
    rm tmp/test5/zero$S.bin
done
rm tmp/test5/gz.txt.gz

cp $srcdir/test5.extr tmp/
for S in 2 3 5 7 11; do
    HASH=$(md5sum "tmp/test5/zero$S.gz" | cut -d' ' -f 1)
    SIZE=$(ls "tmp/test5/zero$S.gz" -l | cut -d' ' -f 5)
    sed tmp/test5.extr -i \
	-e "s,{HASH$S},$HASH," \
	-e "s,{SIZE$S},$SIZE,"
done

$srcdir/test.sh test5

