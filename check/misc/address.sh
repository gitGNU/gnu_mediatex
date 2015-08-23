#!/bin/bash
#=======================================================================
# * Version: $Id: address.sh,v 1.5 2015/08/23 23:39:10 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for address.c
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
misc/ut$TEST > misc/$TEST.out 2>&1

# compare with the expected output
mrProperOutputs misc/$TEST.out
HOST=$(hostname -f)
sed misc/$TEST.out -i \
	-e "s/$HOST/localhost/" \
	-e "s/localhost.localdomain/localhost/"
diff $srcdir/misc/$TEST.exp misc/$TEST.out
