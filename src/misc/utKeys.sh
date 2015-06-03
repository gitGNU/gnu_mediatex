#!/bin/bash
#=======================================================================
# * Version: $Id: utKeys.sh,v 1.3 2015/06/03 14:03:50 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * Unit test script for keys.c
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

# retrieve environment
[ -z $srcdir ] && srcdir=.
#. ${srcdir}/../utMediatex.sh

# run units tests
rm -f key key.pub
ssh-keygen -N "" -t rsa -f key >/dev/null
ssh-keygen -l -f key.pub | awk '{print $2}' >ut.txt
./utkeys -i key.pub 2>/dev/null >ut.out

# compare with the expected outputs
#mrProperOutputs
diff ut.txt ut.out
