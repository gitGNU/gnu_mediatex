#!/bin/bash
#=======================================================================
# * Version: $Id: utUdp.sh,v 1.1 2014/10/13 19:39:46 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for udp.c
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

# retrieve environment
[ -z $srcdir ] && srcdir=.
. ${srcdir}/../utMediatex.sh

# run the unit test
./utudp > ut.out 2>&1 &
./utudp -c > /dev/null 2>&1

# compare with the expected output
sed -i -e "s,(from 127.0.0.1:.*),(from 127.0.0.1:XXX)," ut.out
mrProperOutputs
diff $srcdir/utUdp.exp ut.out
