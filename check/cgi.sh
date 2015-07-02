#!/bin/bash
#=======================================================================
# * Version: $Id: cgi.sh,v 1.1 2015/07/01 10:49:22 nroche Exp $
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for cgi.c
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
. utmediatex.sh # must-be for make distcheck

TEST=$(basename $0)
TEST=${TEST%.sh}

echo "{top template}" > \
    ${HOME}/${MDTXUSER}-coll1/public_html/cgiHeader.shtml
echo "{bottom template}" > \
    ${HOME}/${MDTXUSER}-coll1/public_html/footer.html

# run the unit test
REQUEST_METHOD=GET \
QUERY_STRING="hash=40485334450b64014fd7a4810b5698b3&size=12" \
SCRIPT_FILENAME=/${MDTXUSER}-coll1/public_html/cgi/get.cgi \
MDTX_NO_REGRESSION=1 \
../src/$TEST -s info -f file >$TEST.out 2>&1

# compare with the expected output
mrProperOutputs $TEST.out
diff $srcdir/$TEST.exp $TEST.out