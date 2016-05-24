#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for setuid.c
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

if [ $(id -u) -ne 0 ]; then
    echo -n "(root needed for this test) "
    exit 77 # SKIP
fi

# retrieve environment
[ -z $srcdir ] && srcdir=.
. ${srcdir}/../check/utmediatex.sh

chown root.root misc/utsetuid
chmod u+s misc/utsetuid

adduser \
 --quiet --system --group --shell /bin/bash --no-create-home \
 ut-mdtx-user1 
adduser \
 --quiet --system --group --shell /bin/bash --no-create-home \
 ut-mdtx-user2

addgroup ut-mdtx-user1 ut-mdtx-user2 >/dev/null

su ut-mdtx-user1 -c \
    "misc/utsetuid -s notice -sdebug:script -u ut-mdtx-user2 \
-i $PWD/$srcdir/misc/user.sh" \
    >misc/setuid.out 2>&1

deluser --quiet ut-mdtx-user2 2>/dev/null
deluser --quiet ut-mdtx-user1
delgroup --quiet ut-mdtx-user2

# compare with the expected output
mrProperOutputs misc/setuid.out
diff $srcdir/misc/setuid.exp misc/setuid.out

