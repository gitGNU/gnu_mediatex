#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: deliver.sh,v 1.1 2014/10/13 19:38:33 nroche Exp $
# * Project: MediaTex
# * Module : scripts
# *
# * This script deliver the extracted archives
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

[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/lib
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

Debug "deliver"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"
[ ! -z $1 ] || Error "expect a label as first parameter"
[ ! -z $2 ] || Error "expect a mail address as second parameter"
[ ! -z $3 ] || Error "expect a delay as third parameter"
[ ! -z $4 ] || Error "expect a file name as fourth parameter"
[ ! -z $5 ] || Error "expect an url as fifth parameter"

COLL=$1
ADDRESS=$2
AVAILABLE=$3
FILE=$4
URL=$5

USER="$MDTX-$1"
NAME=$(echo $ADDRESS | cut -d "@" -f1)
SUBJECT="$USER delivery"

/usr/bin/mail $ADDRESS -s "$SUBJECT" <<EOF
Dear $NAME,

The "$FILE" file you requested
is available for $AVAILABLE days at this url :

$URL

Enjoy browsing $COLL collection.
The $MDTX server's team.
$([ ! -f /usr/games/cowsay ] || /usr/games/cowsay mheu)
EOF

Info "done"