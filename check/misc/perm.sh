#!/bin/bash
#=======================================================================
# * Version: $Id: perm.sh,v 1.2 2015/08/23 23:39:12 nroche Exp $
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

# unit test's sand box
mkdir -p $TMP
touch $TMP/sand-box.txt

TEST=$(basename $0)
TEST=${TEST%.sh}

# run units tests

# internal tests
misc/ut$TEST -d $PWD/$srcdir -w $PWD \
    >misc/$TEST.out 2>&1

# tests using parameters
misc/ut$TEST -d /usr/bin -u foo -g root -p 755 \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d /usr/bin -u root -g bar -p 755 \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d /usr/bin -u root -g root -p 777 \
    >>misc/$TEST.out 2>&1 || /bin/true

misc/ut$TEST -d /usr/bin -u root -g root -p 755 \
    >>misc/$TEST.out 2>&1

# test other test will not fails using no regression mode
MDTX_NO_REGRESSION=1 \
    misc/ut$TEST -d /usr/bin -u root -g root -p 777 \
    >>misc/$TEST.out 2>&1 || /bin/true

MDTX_NO_REGRESSION=1 \
    misc/ut$TEST -d /usr/bin -u foo -g bar -p 755 \
    >>misc/$TEST.out 2>&1

# suppress the current date
sed -i -e "s/\(current date: \).*/\1 XXX/" misc/$TEST.out

# compare with the expected output
mrProperOutputs misc/$TEST.out
diff $srcdir/misc/$TEST.exp misc/$TEST.out

