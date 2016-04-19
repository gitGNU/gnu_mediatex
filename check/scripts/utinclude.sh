#!/bin/bash
#=======================================================================
# * Version: $Id: utinclude.sh,v 1.1 2015/07/01 10:50:04 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * load mediatex path and variables
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

MDTX_MDTXUSER="mdtx"

# includes
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ]     || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
source $srcdir/utscripts.sh

UNIT_TEST_start "include"
Debug "test for include.sh"

echo -e "\n* Autotools inputs:"
echo "MEDIATEX=$MEDIATEX"
echo "BINDIR=$BINDIR"
echo "DATAROOTDIR=$DATAROOTDIR"
echo "EXEC_PREFIX=$EXEC_PREFIX"
echo "LOCALSTATEDIR=$LOCALSTATEDIR"
echo "PREFIX=$PREFIX"
echo "SYSCONFDIR=$SYSCONFDIR"

echo -e "\n* absolute paths:"
echo "ETCDIR=$ETCDIR"
echo "DATADIR=$DATADIR"
echo "STATEDIR=$STATEDIR"
echo "CACHEDIR=$CACHEDIR"
echo "PIDDIR=$PIDDIR"
echo "SCRIPTS=$SCRIPTS"
echo "MISC=$MISC"
echo "HOSTSSH=$HOSTSSH"

echo -e "\n* recurent variables:"
echo "CVSROOT=$CVSROOT"
echo "MDTXHOME=$MDTXHOME"
echo "MD5SUMS=$MD5SUMS"
echo "CACHES=$CACHES"
echo "EXTRACT$EXTRACT"
echo "CVSCLT=$CVSCLT"
echo "JAIL=$JAIL"
echo "HOMES=$HOMES"
echo "MDTXCVS=$MDTXCVS"
echo "CONFFILE=$CONFFILE"
echo "PIDFILE=$PIDFILE"

echo -e "\n* Command line inputs:"
echo "LOG_FILE=$LOG_FILE"
echo "LOG_FACILITY=$LOG_FACILITY"
echo "LOG_SEVERITY=$LOG_SEVERITY"
echo "DRY_RUN=$DRY_RUN"
echo "MDTX=$MDTX"

Info "success"
UNIT_TEST_stop "include"
