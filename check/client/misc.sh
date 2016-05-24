#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module:  client modules (User API)
# *
# * Unit test script for misc.c
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

# run the unit test
client/ut$TEST >client/$TEST.out 2>&1

[ -z "$1" ] || {
    cat >${HOME}/mdtx1-coll1/public_html/.htaccess <<EOF
# Server side includes
Options +Includes
DirectoryIndex index.shtml
EOF

    echo "please try browsing: "
    echo " file://${HOME}/mdtx1-coll1/public_html/"
    echo "or:"
    echo " $ cp -r ${HOME}/mdtx1-coll1/public_html/ /var/www/html/ut"
    echo " $ chown www-data -R /var/www/html/ut"
    echo "modify apache configuration:"
    echo "        <Directory /var/www/html/ut>"
    echo "            Require all granted"
    echo "            AllowOverride All"
    echo "        </Directory>"
    echo "and browse:"
    echo " http://localhost/ut/"
    exit 0
}

# compare with the expected output
mrProperOutputs client/$TEST.out
sed -i client/$TEST.out -e "s/\(audit_20100101-010000_\).*/\1_XXX.txt)/"
diff $srcdir/client/$TEST.exp client/$TEST.out
