#!/bin/bash
#set -x
set -e
#=======================================================================
# * Project: MediaTex
# * Module : scripts
# *
# * This script setup a new MediaTex collection
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

[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/lib
[ ! -z $MDTX_SH_INCLUDE ]  || source $libdir/include.sh
[ ! -z $MDTX_SH_USERS ]    || source $libdir/users.sh
[ ! -z $MDTX_SH_SSH ]      || source $libdir/ssh.sh
[ ! -z $MDTX_SH_GIT ]      || source $libdir/git.sh
[ ! -z $MDTX_SH_JAIL ]     || source $libdir/jail.sh
[ ! -z $MDTX_SH_HTDOCS ]   || source $libdir/htdocs.sh

Debug "new"
[ $(id -u) -eq 0 ] || Error "need to be root"
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"
[ ! -z $1 ] || Error "expect a label as first parameter"

SERV=$(echo $1 | cut -s -d "-" -f1)
TMP=$(echo $1 | cut -d "-" -f2)
COLL=$(echo $TMP | cut -d "@" -f1)
TMP=$(echo $1 | cut -s -d "@" -f2)
HOST=$(echo $TMP | cut -d ":" -f1)
PORT=$(echo $TMP | cut -s -d ":" -f2)
[ "$COLL" = "mdtx" ] && Error "collection cannot be labeled mdtx"
[ -z "$SERV" ] && SERV=$MDTX
[ -z "$HOST" ] && HOST="localhost"
[ -z "$PORT" ] && PORT=22
USER=$MDTX-$COLL
MUSER=$SERV-$COLL

# new user and his key
USERS_coll_create_user $USER
SSH_build_key $USER
HTDOCS_configure_coll_apache2 $USER

# setup a new repository if hosted localy (master host)
if [ \( "$SERV" = "$MDTX" \) -a \( "$HOST" = "localhost" \) ]; then
    SSH_bootstrapKeys $USER
    GIT_coll_import $USER
    JAIL_bind
fi

# setup connexion
SSH_configure_client $USER $HOST $PORT
JAIL_add_user $USER
JAIL_bind

# test the connection
set +e
su $USER 2>/dev/null <<EOF 
ssh -o PasswordAuthentication=no $MUSER@$HOST ls >/dev/null
EOF
RC=$?
set -e
if [ $RC -ne 0 ]; then
    if [ $MDTX_KEY_HAVE_CHANGE -eq 1 ]; then
	Notice "new public key"
    fi
    Notice "public key: $HOMES/$USER$CONF_SSHDIR/id_dsa.pub"
    exit 0
fi

# checkout the collection
GIT_coll_checkout $USER $SERV $COLL $HOST
HTDOCS_configure_coll_cgit $USER

# BUG too ?
# reload daemons as there configuration have changed
/usr/sbin/invoke-rc.d apache2 reload

# BUG (to remove when ACL will be implemented)
#if [ $MDTX = mdtx ]; then
#    /usr/sbin/invoke-rc.d ${MEDIATEX#/}d restart
#fi

Info "done"
