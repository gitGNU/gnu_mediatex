#!/bin/bash
#=======================================================================
# * Version: $Id: utConfTree.sh,v 1.3 2015/06/03 14:03:41 nroche Exp $
# * Project: MediaTex
# * Module:  memory tree modules
# *
# * Unit test script for conftree.c
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

[ -z $srcdir ] && srcdir=.

# retrieve environment
. ${srcdir}/../utMediatex.sh

# $1 : $MDTXUSER (mdtx => mdtx[1-3])
function populateConfiguration()
{
    # simulate directory build during installation
    install -m 755 -d $TMPDIR
    install -m 755 -d $SYSCONFDIR
    install -m 755 -d $DATAROOTDIR
    install -m 755 -d $LOCALSTATEDIR
    install -m 755 -d $ETCDIR
    install -m 755 -d $DATADIR
    install -m 755 -d $STATEDIR
    install -m 755 -d $CACHEDIR
    install -m 755 -d $PIDDIR
    install -m 755 -d $SCRIPTS
    install -m 755 -d $EXAMPLES

    loadPaths $1

    install -m 755 -d $HOME
    install -m 755 -d $MDTXHOME
    install -m 755 -d $MDTXHOME/jail
    install -m 750 -d $MD5SUMS
    install -m 750 -d $CACHES
    install -m 750 -d $EXTRACT
    install -m 750 -d $CVSROOT
    install -m 770 -d $CVSROOT/CVSROOT
    install -m 750 -d $CVSROOT/$MDTXUSER
    install -m 750 -d $CVSCLT
    install -m 770 -d $MDTXCVS
    install -m 755 -d $HOSTSSH

    for COLL in $(seq 1 3); do
	populateCollection "$1-coll${COLL}"
    done

    # host key (may not differ for each server when hosted togethers):
    install -m 644 ${srcdir}/hostKey_rsa.pub $HOSTSSH/ssh_host_rsa_key.pub
    install -m 600 ${srcdir}/hostKey_rsa $HOSTSSH/ssh_host_rsa_key

    # collection keys (randomized but should ever be same on each hosts):

    TARGET=$HOME/$1-coll1
    install -m 644 ${srcdir}/user1Key_rsa.pub $TARGET/.ssh/id_rsa.pub
    install -m 600 ${srcdir}/user1Key_rsa $TARGET/.ssh/id_rsa

    TARGET=$HOME/$1-coll2
    install -m 644 ${srcdir}/user2Key_rsa.pub $TARGET/.ssh/id_rsa.pub
    install -m 600 ${srcdir}/user2Key_rsa $TARGET/.ssh/id_rsa

    TARGET=$HOME/$1-coll3
    install -m 644 ${srcdir}/user3Key_dsa.pub $TARGET/.ssh/id_dsa.pub
    install -m 600 ${srcdir}/user3Key_dsa $TARGET/.ssh/id_dsa
}

# build directories
for SERVER in $(seq 1 3); do
    populateConfiguration "${lMDTXUSER}${SERVER}"
done

# run the unit test
./utconfTree > ut.out 2>&1

# compare with the expected output
loadPaths "${lMDTXUSER}2"
diff -I '# Version: $Id' \
    $srcdir/utConfTree.exp ${MDTXCVS}/${MDTXUSER}.conf
mrProperOutputs
diff $srcdir/utConfTree.exp2 ut.out
