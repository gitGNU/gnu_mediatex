#!/bin/bash
#=======================================================================
# * Project: Jukebox
# * Module: tests
# *
# * Unit test script for splitting
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

# create 2 big files to split
rm -fr tmp/test3
mkdir -p tmp/test3
touch tmp/test3/gz.txt
gzip tmp/test3/gz.txt
dd bs=1048576 count=2 if=/dev/zero of=tmp/test3/zero.bin
cat tmp/test3/gz.txt.gz tmp/test3/zero.bin > tmp/test3/zero1.gz
cp tmp/test3/zero1.gz tmp/test3/zero2.gz
rm tmp/test3/gz.txt.gz tmp/test3/zero.bin
echo "1" >> tmp/test3/zero1.gz
echo "2" >> tmp/test3/zero2.gz

$srcdir/test.sh test3
