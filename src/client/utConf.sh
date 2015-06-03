#!/bin/bash
#=======================================================================
# * Version: $Id: utConf.sh,v 1.3 2015/06/03 14:03:31 nroche Exp $
# * Project: MediaTex
# * Module:  client modules (User API)
# *
# * Unit test script for conf.c
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

# run the unit test
populateCollection ${MDTXUSER}-coll4

cp $CVSCLT/$MDTXUSER-coll1$EXTRFILE $CVSCLT/$MDTXUSER-coll2$EXTRFILE
cp $CVSCLT/$MDTXUSER-coll1$EXTRFILE $CVSCLT/$MDTXUSER-coll3$EXTRFILE
cp $CVSCLT/$MDTXUSER-coll1$EXTRFILE $CVSCLT/$MDTXUSER-coll4$EXTRFILE

cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll2$SERVFILE
cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll3$SERVFILE
cp $CVSCLT/$MDTXUSER-coll1$SERVFILE $CVSCLT/$MDTXUSER-coll4$SERVFILE

cp $CVSCLT/$MDTXUSER-coll1$CATHFILE $CVSCLT/$MDTXUSER-coll4$CATHFILE

TARGET=$HOME/$MDTXUSER-coll4
install -m 644 ${srcdir}/../memory/user3Key_dsa.pub $TARGET/.ssh/id_dsa.pub
install -m 600 ${srcdir}/../memory/user3Key_dsa $TARGET/.ssh/id_dsa

./utconf >ut.out 2>&1

# compare with the expected output
mrProperOutputs
diff $srcdir/utConf.exp ut.out
