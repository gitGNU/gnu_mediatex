#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for perm.c
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

# unit test's sand box (for distcheck)
mkdir -p $TMP
touch $TMP/sand-box.txt

TEST=$(basename $0)
TEST=${TEST%.sh}

# run units tests

# internal tests
misc/ut$TEST -d $PWD/$srcdir -w $PWD \
    >misc/$TEST.out 2>&1

# tests using parameters
BASE_ACL="-m u::rwx -m g::rwx -m o::--- -m m:rwx"
DIR=$TMP/acl
install -o $USER -g $USER -m 770 -d $DIR
setfacl $BASE_ACL -m "u:www-data:r-x" $DIR

misc/ut$TEST -d $DIR -u foo -g $USER \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d $DIR -u $USER -g bar \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d $DIR -u $USER -g $USER -P 755 \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d $DIR -u $USER -g $USER -a "u:www-data:r--" \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d $DIR -u $USER -g $USER -a "u:www-data:r-x" \
    >>misc/$TEST.out 2>&1 || /bin/true

setfacl -d $BASE_ACL -m "u:www-data:r-x" $DIR
misc/ut$TEST -d $DIR -u $USER -g $USER -a "u:www-data:r-x" \
    >>misc/$TEST.out 2>&1

rmdir $DIR

# suppress the current date and user
sed misc/$TEST.out -i \
    -e "s/\(current date: \).*/\1 XXX/" \
    -e "s/\ \($(id -un)\)/ USER/g" 

# compare with the expected output
mrProperOutputs misc/$TEST.out
diff $srcdir/misc/$TEST.exp misc/$TEST.out

