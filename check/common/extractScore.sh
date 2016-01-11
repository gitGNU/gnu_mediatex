#!/bin/bash
#=======================================================================
# * Version: $Id: extractScore.sh,v 1.2 2015/07/28 11:45:39 nroche Exp $
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for extractscore.c
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

# run the unit tests
common/ut$TEST -sdebug:common >common/$TEST.out 2>&1

# compare with the expected output
mrProperOutputs common/$TEST.out
diff $srcdir/common/$TEST.exp common/$TEST.out \
    -I '# Version: $Id'
