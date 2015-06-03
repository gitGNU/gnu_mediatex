#!/bin/bash
#=======================================================================
# * Version: $Id: utCvsPrint.sh,v 1.3 2015/06/03 14:03:41 nroche Exp $
# * Project: MediaTex
# * Module:  memory tree modules
# *
# * Unit test script for cvsprint.c
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
set -e
#set -x

[ -z $srcdir ] && srcdir=.

# retrieve environment
. ${srcdir}/../utMediatex.sh

# run the unit test
./utcvsPrint > ut.out 2>&1

# compare with the expected outputs
mrProperOutputs
diff $srcdir/utCvsPrint.exp ut.out


