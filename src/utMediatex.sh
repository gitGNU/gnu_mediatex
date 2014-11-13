#!/bin/bash
#=======================================================================
# * Version: $Id: utMediatex.sh,v 1.2 2014/11/13 16:36:15 nroche Exp $
# * Project: MediaTex
# * Module : C codes
# *
# * This script is include by all C unit tests
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

[ -z $srcdir ] && srcdir=.
CONFIG=../../config.h # do not prefix by ${srcdir} (important for distcheck)
HEADER=${srcdir}/../mediatex.h

# this script should alway be called from one directory deeper
# ex: ../utMediatex.sh
# bellow is a workaround to call it directely, so as to debug it
if [ "$0" == "./utMediatex.sh" ]; then
    CONFIG=../config.h
    HEADER=mediatex.h
fi

# from config.h
lSYSCONFDIR=$(grep "^#define CONF_SYSCONFDIR" $CONFIG | cut -d'"' -f2)
lDATAROOTDIR=$(grep "^#define CONF_DATAROOTDIR" $CONFIG | cut -d'"' -f2)
lLOCALSTATEDIR=$(grep "^#define CONF_LOCALSTATEDIR" $CONFIG | cut -d'"' -f2)
lMEDIATEX=$(grep -m 1 "^#define CONF_MEDIATEXDIR" $CONFIG | cut -d'"' -f2)

# from mediatex.h
lMDTXUSER=$(grep "^#define DEFAULT_MDTXUSER" $HEADER | cut -d'"' -f2)
lTMPDIR=$(grep "^#define CONF_TMPDIR" $HEADER | cut -d'"' -f2)
lSTATEDIR=$(grep "^#define CONF_STATEDIR" $HEADER | cut -d'"' -f2)
lCACHEDIR=$(grep "^#define CONF_CACHEDIR" $HEADER | cut -d'"' -f2)
lPIDDIR=$(grep "^#define CONF_PIDDIR" $HEADER | cut -d'"' -f2)
lSCRIPTS=$(grep "^#define CONF_SCRIPTS" $HEADER | cut -d'"' -f2)
lEXAMPLES=$(grep "^#define CONF_EXAMPLES" $HEADER | cut -d'"' -f2)
lHOSTSSH=$(grep "^#define CONF_HOSTSSH" $HEADER | cut -d'"' -f2)
lCVSROOT=$(grep "^#define CONF_CVSROOT" $HEADER | cut -d'"' -f2)
lMD5SUMS=$(grep "^#define CONF_MD5SUMS" $HEADER | cut -d'"' -f2)
lCACHES=$(grep "^#define CONF_CACHES" $HEADER | cut -d'"' -f2)
lEXTRACT=$(grep "^#define CONF_EXTRACT" $HEADER | cut -d'"' -f2)
lHOME=$(grep "^#define CONF_HOME" $HEADER | cut -d'"' -f2)
lCVSCLT=$(grep "^#define CONF_CVSCLT" $HEADER | cut -d'"' -f2)
lCONFFILE=$(grep "^#define CONF_CONFFILE" $HEADER | cut -d'"' -f2)
lPIDFILE=$(grep "^#define CONF_PIDFILE" $HEADER | cut -d'"' -f2)
lSUPPFILE=$(grep "^#define CONF_SUPPFILE" $HEADER | cut -d'"' -f2)
lSERVFILE=$(grep "^#define CONF_SERVFILE" $HEADER | cut -d'"' -f2)
lCATHFILE=$(grep "^#define CONF_CATHFILE" $HEADER | cut -d'"' -f2)
lEXTRFILE=$(grep "^#define CONF_EXTRFILE" $HEADER | cut -d'"' -f2)

#  prefix to add
TMPDIR=${lTMPDIR}${lMDTXUSER}
SYSCONFDIR=${TMPDIR}${lSYSCONFDIR}
DATAROOTDIR=${TMPDIR}${lDATAROOTDIR}
LOCALSTATEDIR=${TMPDIR}${lLOCALSTATEDIR}

#  absolute paths
ETCDIR=${SYSCONFDIR}${lMEDIATEX}
DATADIR=${DATAROOTDIR}${lMEDIATEX}
STATEDIR=${LOCALSTATEDIR}${lSTATEDIR}${lMEDIATEX}
CACHEDIR=${LOCALSTATEDIR}${lCACHEDIR}${lMEDIATEX}
PIDDIR=${LOCALSTATEDIR}${lPIDDIR}${lMEDIATEX}
SCRIPTS=${DATADIR}${lSCRIPTS}
EXAMPLES=${DATADIR}${lEXAMPLES}
HOSTSSH=${SYSCONFDIR}${lHOSTSSH}

# files with relatives path
SUPPFILE="${lSUPPFILE}"
SERVFILE="${lSERVFILE}.txt"
CATHFILE="${lCATHFILE}00.txt"
EXTRFILE="${lEXTRFILE}00.txt"

# $1 : $lMDTXUSER+... (mdtx => mdtx[1-3])
function loadPaths()
{
    MDTXUSER="$1"
    CVSROOT="${STATEDIR}/$1"
    MDTXHOME="${CACHEDIR}/$1"
    MD5SUMS="${MDTXHOME}${lMD5SUMS}"
    CACHES="${MDTXHOME}${lCACHES}"
    EXTRACT="${MDTXHOME}${lEXTRACT}"
    CVSCLT="${MDTXHOME}${lCVSCLT}"
    HOME="${MDTXHOME}${lHOME}"
    MDTXCVS="${CVSCLT}/$1"
    CONFFILE="${MDTXCVS}/$1${lCONFFILE}"
    PIDFILE="${PIDDIR}/$1${lPIDFILE}"
}

# $1 : collection label ("${MDTXUSER}-test${COLL}")
function populateCollection()
{
    TARGET=$HOME/$1

    # simulate collection directory build during creation
    install -m 750 -d $TARGET
    install -m 770 -d $TARGET/cvs
    install -m 700 -d $TARGET/.ssh
    install -m 750 -d $TARGET/public_html
    install -m 750 -d $TARGET/public_html/index
    install -m 750 -d $TARGET/public_html/cache
    install -m 750 -d $TARGET/public_html/score
    install -m 750 -d $TARGET/public_html/cgi

    install -m 750 -d $CACHES/$1
    install -m 770 -d $EXTRACT/$1
    install -m 750 -d $CVSROOT/$1
    install -m 770 -d $CVSCLT/$1

    touch $CVSCLT/$1$CATHFILE
    touch $CVSCLT/$1$EXTRFILE
}

# so as to pass unit test easier
function mrProperOutputs()
{
    # hide line numbers
    sed -i -e "s,^\(\[.*\):.*\],\1]," ut.out

    # hide base directories path
    sed -i -e "s,$SYSCONFDIR,SYSCONFDIR,g" ut.out
    sed -i -e "s,$LOCALSTATEDIR,LOCALSTATEDIR,g" ut.out
    sed -i -e "s,$DATAROOTDIR,DATAROOTDIR,g" ut.out
    sed -i -e "s,\.\./\.\./\.\./src/[^/]*/,./,g" ut.out

    # hide uid
    sed -i -e "s,\(logoutUser\).*,\1 XXX," ut.out

    # hide cvs versionning id
    sed -i -e "s,^\(# Version: \).*,\1 XXX," ut.out
}

# finaly (default values for mdtx1)
loadPaths "${lMDTXUSER}1"

# do not do that when call by other unit-tests
if [ "$0" == "./utMediatex.sh" ]; then
    echo "MEDIATEX = $MEDIATEX"
    echo "MDTXUSER = $MDTXUSER"
    echo "TMPDIR = $TMPDIR"
    echo "SYSCONFDIR = $SYSCONFDIR"
    echo "DATAROOTDIR = $DATAROOTDIR"
    echo "LOCALSTATEDIR = $LOCALSTATEDIR"

    echo "ETCDIR = $ETCDIR"
    echo "DATADIR = $DATADIR"
    echo "STATEDIR = $STATEDIR"
    echo "CACHEDIR = $CACHEDIR"

    echo "HOSTSSH = $HOSTSSH"
    echo "PIDDIR = $PIDDIR"
    echo "CVSROOT = $CVSROOT"
    echo "SCRIPTS = $SCRIPTS"
    echo "EXAMPLES = $EXAMPLES"
    echo "MD5SUMS = $MD5SUMS"
    echo "CACHES = $CACHES"
    echo "EXTRACT = $EXTRACT"
    echo "CVSCLT = $CVSCLT"
    echo "HOME = $HOME"
    echo "MDTXCVS = $MDTXCVS"

    echo "CONFFILE = $CONFFILE"
    echo "SUPPFILE = $SUPPFILE"
    echo "SERVFILE = $SERVFILE"
    echo "CATHFILE = $CATHFILE"
    echo "EXTRFILE = $EXTRFILE"
    echo "PIDFILE = $PIDFILE"
fi
