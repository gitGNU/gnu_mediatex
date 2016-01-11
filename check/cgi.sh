#!/bin/bash
#=======================================================================
# * Version: $Id: cgi.sh,v 1.5 2015/10/01 21:52:39 nroche Exp $
# * Project: MediaTex
# * Module:  common modules (both used by clients and server)
# *
# * Unit test script for cgi.c
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
. utmediatex.sh # must-be for make distcheck

TEST=$(basename $0)
TEST=${TEST%.sh}

# run the unit test
echo "{top template}" > \
    ${HOME}/${MDTXUSER}-coll1/public_html/cgiHeader.shtml
echo "{bottom template}" > \
    ${HOME}/${MDTXUSER}-coll1/public_html/footer.html

echo "** first query: asking for content" >$TEST.out
REQUEST_METHOD=GET \
QUERY_STRING="hash=40485334450b64014fd7a4810b5698b3&size=12" \
SCRIPT_FILENAME=/${MDTXUSER}-coll1/public_html/cgi/get.cgi \
MDTX_NO_REGRESSION=1 \
    ../src/$TEST -n -s info -f file >>$TEST.out 2>&1

echo "** second query: register a mail address" >>$TEST.out
REQUEST_METHOD=POST \
SCRIPT_FILENAME=/${MDTXUSER}-coll1/public_html/cgi/get.cgi \
MDTX_NO_REGRESSION=1 \
CONTENT_TYPE=application/x-www-form-urlencoded \
CONTENT_LENGTH=64 \
    ../src/$TEST -n -s info -f file >>$TEST.out 2>&1 <<EOF
hash=40485334450b64014fd7a4810b5698b3&size=12&mail=test@test.org
EOF

# compare with the expected output
mrProperOutputs $TEST.out
diff $srcdir/$TEST.exp $TEST.out

# note for gdb:
# M-x gdb
# Run gdb (like this): libtool --mode=execute gdb --annotate=3 ../src/cgi
# > set env REQUEST_METHOD=GET
# > set env QUERY_STRING=hash=40485334450b64014fd7a4810b5698b3&size=12
# > set env SCRIPT_FILENAME=/mdtx1-coll1/public_html/cgi/get.cgi
# > set env MDTX_NO_REGRESSION=1
# > set args -n -s debug
