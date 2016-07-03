#!/bin/bash
#=======================================================================
# * Version: $Id: search.cgi,v 1.1.1.1 2015/05/31 14:39:44 nroche Exp $
# * Project: MediaTex
# * Module : Upload bash script
# *
# * This script is part of the collection meta-data
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
#set -x
set -e

QUERY="mediatex upload++"

USER=${PWD%/public_html/cgi}
USER=${USER##*/}
SERVER=${USER%-*}
COLL=${USER#*-}

#echo "$QUERY -c $SERVER $@ to coll $COLL"
$QUERY -c $SERVER $@ to coll $COLL -p -ffile 2>&1
