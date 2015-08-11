#!/bin/bash
#=======================================================================
# * Version: $Id: upload.sh,v 1.4 2015/08/11 18:14:22 nroche Exp $
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
cat >client/$TEST.cat <<EOF
Category "css": "drawing"

Document "css": "css"
  With "designer" = "Me" ""
  b281449c229bcc4a3556cdcc0d3ebcec:815
EOF

cat >client/$TEST.ext <<EOF
(ISO
0a7ecd447ef2acb3b5c6e4c550e6636f:374784
=>
c0c055a0829982bd646e2fafff01aaa6:4066	logoP2.cat
)
EOF

client/ut$TEST \
    -C client/$TEST.cat \
    >client/$TEST.out 2>&1

client/ut$TEST \
    -E client/$TEST.ext \
    >>client/$TEST.out 2>&1

client/ut$TEST \
    -C client/$TEST.cat \
    -E client/$TEST.ext \
    >>client/$TEST.out 2>&1

# compare with the expected output
mrProperOutputs client/$TEST.out
diff $srcdir/client/$TEST.exp client/$TEST.out
