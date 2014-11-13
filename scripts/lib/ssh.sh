#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: ssh.sh,v 1.2 2014/11/13 16:36:13 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx's ssh keys.
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

###
# Rq: it helps a lot !
# # /usr/sbin/sshd -D -ddd
# # ssh -vvv bibi
###

# includes
MDTX_SH_SSH=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

MDTX_KEY_HAVE_CHANGE=0

# build ssh keys without password
# $1: collection user
function SSH_build_key()
{
    Debug "$FUNCNAME: $1" 2
    [ ! -z "$1" ] || error "need a collection label"
    SSH_KNOWNHOSTS=$CACHEDIR/$MDTX/home/$1/.ssh/known_hosts
    COMMENT="$USER@$(hostname -f)"
    cd $CACHEDIR/$MDTX/home/$1/.ssh

    if [ \( -e id_dsa \) -a \( -e id_dsa.pub \) ]; then
	Info "re-use the previous keys we found"
    else 
	MDTX_KEY_HAVE_CHANGE=1
	ssh-keygen -t dsa -f id_dsa -P "" -C "$COMMENT" >/dev/null 2>&1 ||
	Error "cannot generate ssh keys"
    fi
    chmod 644 id_dsa.pub
    chmod 600 id_dsa
    chown $1:$1 *
    cd - >/dev/null

    # reset known servers keys
    rm -f $SSH_KNOWNHOSTS
}

# bootstrap ssh configuration with local keys
# $1: collection user
function SSH_bootstrapKeys()
{
    Debug "$FUNCNAME $1 $2" 2
    cd $CACHEDIR/$MDTX/home/$1/.ssh

    # add a public key to accept login
    install -m 644 id_dsa.pub authorized_keys

    # add a server key to accept connection 
    echo -n "localhost " > known_hosts
    cat /etc/ssh/ssh_host_rsa_key.pub >> known_hosts
    ssh-keygen -H -f known_hosts 2>&1
    rm -f known_hosts.old
    chmod 644 known_hosts

    chown $1:$1 authorized_keys
    chown $1:$1 known_hosts
    cd - >/dev/null
}

# configure ssh without host checking in the curent directory
# $1: user
# $2: host
# $3: port
function SSH_configure_client()
{
    Debug "$FUNCNAME $1 $2 $3" 2
    SSH_CONFIG=$CACHEDIR/$MDTX/home/$1/.ssh/config

    echo -e "Host *\n" > $SSH_CONFIG
    echo -e " VerifyHostKeyDNS yes\n" >> $SSH_CONFIG
    echo -e " HostKeyAlgorithms ssh-rsa,ssh-dss\n\n" >> $SSH_CONFIG
    echo -e "Host $2:\n\tPort $3\n" >> $SSH_CONFIG
    chown $1:$1 $SSH_CONFIG
}

# enable ssh login into chroot
# $1: yes or no
function SSH_chroot_login()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"
    JAIL=$CACHEDIR/$MDTX/jail
    PATTERN="$MDTX-*"

    # protect stars from shell expension
    TMP_STRING="/# <<<$PATTERN/,/# $PATTERN>>>/ d"
    SED_STRING=$(echo $TMP_STRING | sed -e 's/*/\\\*/g')
    sed -i -e "$SED_STRING" /etc/ssh/sshd_config
    if [ "$1" = "yes" ]; then
	cat >> /etc/ssh/sshd_config <<EOF
# <<<$PATTERN
Match user $PATTERN
          ChrootDirectory $JAIL
          X11Forwarding no
          AllowTcpForwarding no
# $PATTERN>>>
EOF
    fi
    invoke-rc.d ssh reload
}

# unitary tests
if UNIT_TEST_start "ssh"; then
    [ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh

    MDTX="ut3-mdtx"
    COLL="hello"
    USER="$MDTX-$COLL"

    # cleanup if previous test has failed
    USERS_mdtx_remove_user
    USERS_coll_remove_user $USER
    USERS_mdtx_disease

    # cf init.sh
    USERS_root_populate
    USERS_mdtx_create_user

    # cf new.sh
    USERS_coll_create_user $USER
    SSH_build_key $USER
    SSH_bootstrapKeys $USER
    SSH_configure_client $USER "localhost" 22

    ## check ssh connection
    QUERY="ssh -o PasswordAuthentication=no ${USER}@localhost ls"
    Info "su USER -c \"$QUERY\""
    su $USER -c "$QUERY" || Error "Cannot connect via ssh"
    
    # cf free.sh
    USERS_coll_disease $USER
    USERS_coll_remove_user $USER
    USERS_mdtx_remove_user

    Info "success"
    UNIT_TEST_stop "ssh"
fi

