#!/bin/bash
#=======================================================================
# * Version: $Id: utSsh.sh,v 1.1 2014/10/13 19:39:04 nroche Exp $
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for ssh.c
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
./utssh >ut.out 2>&1

# compare with the expected outputs
mrProperOutputs
diff $srcdir/utSsh.exp ut.out
diff $srcdir/utSsh.exp2 $HOME/${MDTXUSER}-coll1/.ssh/authorized_keys
diff $srcdir/utSsh.exp3 $HOME/${MDTXUSER}-coll1/.ssh/config

rc=1
cmp $srcdir/utSsh.exp4 \
    $HOME/${MDTXUSER}-coll1/.ssh/known_hosts >/dev/null || rc=0
if [ $rc -eq 1 ]; then
    echo ".ssh/known_hosts should change after hash"
    exit 1
fi

