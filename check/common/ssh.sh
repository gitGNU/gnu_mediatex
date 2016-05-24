#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for ssh.c
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

# run the unit test
common/ut$TEST >common/$TEST.out 2>&1

# compare with the expected outputs
mrProperOutputs common/$TEST.out
diff $srcdir/common/$TEST.exp common/$TEST.out
diff $srcdir/common/$TEST.exp2 $HOME/${MDTXUSER}-coll1/.ssh/authorized_keys
diff $srcdir/common/$TEST.exp3 $HOME/${MDTXUSER}-coll1/.ssh/config

rc=1
cmp $srcdir/common/$TEST.exp4 \
    $HOME/${MDTXUSER}-coll1/.ssh/known_hosts >/dev/null || rc=0
if [ $rc -eq 1 ]; then
    echo ".ssh/known_hosts should change after hash"
    exit 1
fi

