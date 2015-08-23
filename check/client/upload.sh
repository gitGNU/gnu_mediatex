#!/bin/bash
#=======================================================================
# * Version: $Id: upload.sh,v 1.7 2015/08/23 23:39:09 nroche Exp $
# * Project: MediaTex
# * Module:  client modules (User API)
# *
# * Unit test script for upload.c
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
. utmediatex.sh

TEST=$(basename $0)
TEST=${TEST%.sh}

# run the unit test
CONTENT1=$(md5sum $srcdir/../misc/mediatex.css | cut -d' ' -f 1)
CONTENT1=$CONTENT1:$(ls $srcdir/../misc/mediatex.css -l | cut -d' ' -f 5)
CONTENT2=$(md5sum $srcdir/../misc/logo.png | cut -d' ' -f 1)
CONTENT2=$CONTENT2:$(ls $srcdir/../misc/logo.png -l | cut -d' ' -f 5)

cat >client/$TEST.cat <<EOF
Category "css": "drawing"

Document "css": "css"
  With "designer" = "Me" ""
  $CONTENT1
EOF

cat >client/$TEST.ext <<EOF
(ISO
$CONTENT1
=>
$CONTENT2 there/and/there
)
EOF

client/ut$TEST \
    >client/$TEST.out 2>&1

client/ut$TEST \
    -C client/$TEST.cat \
    >>client/$TEST.out 2>&1


client/ut$TEST \
    -E client/$TEST.ext \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -F $srcdir/../misc/mediatex.css \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -C client/$TEST.cat \
    -E client/$TEST.ext \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -C client/$TEST.cat \
    -F $srcdir/../misc/mediatex.css \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -E client/$TEST.ext \
    -F $srcdir/../misc/mediatex.css \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -C client/$TEST.cat \
    -E client/$TEST.ext \
    -F $srcdir/../misc/mediatex.css \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -F $srcdir/../misc/mediatex.css \
    -T dirname/ \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -F $srcdir/../misc/mediatex.css \
    -T dirname/filename \
    >>client/$TEST.out 2>&1

# compare with the expected output
mrProperOutputs client/$TEST.out
diff $srcdir/client/$TEST.exp client/$TEST.out
