#!/bin/bash
#=======================================================================
# * Version: $Id: keys.sh,v 1.1 2015/07/01 10:49:50 nroche Exp $
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

TEST=$(basename $0)
TEST=${TEST%.sh}

# run units tests
rm -f misc/key misc/key.pub
ssh-keygen -N "" -t rsa -f misc/key >/dev/null
ssh-keygen -l -f misc/key.pub | awk '{print $2}' >misc/$TEST.txt
misc/utkeys -i misc/key.pub 2>/dev/null >misc/$TEST.out

# compare with the expected outputs
diff misc/$TEST.txt misc/$TEST.out
