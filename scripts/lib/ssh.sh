#!/bin/bash
#=======================================================================
# * Version: $Id: ssh.sh,v 1.4 2015/06/30 17:37:23 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx's ssh keys.
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

###
# Rq: it helps a lot !
# # /usr/sbin/sshd -D -ddd
# # ssh -vvv bibi
###

if [ $(id -u) -ne 0 ]; then
    echo -n "(root needed for this test) "
    exit 77 # SKIP
fi

# includes
MDTX_SH_SSH=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/../scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

MDTX_KEY_HAVE_CHANGE=0

# build ssh keys without password
# $1: collection user
function SSH_build_key()
{
    Debug "$FUNCNAME: $1" 2
    [ ! -z "$1" ] || error "need a collection label"

    # /var/cache/mediatex/mdtx/home/$1/.ssh
    COLL_SSHDIR=$HOMES/$1$CONF_SSHDIR
    SSH_KNOWNHOSTS=$COLL_SSHDIR$CONF_SSHKNOWN
    COMMENT="${1}@$(hostname -f)"

    cd $COLL_SSHDIR

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

    # /var/cache/mediatex/mdtx/home/$1/.ssh
    COLL_SSHDIR=$HOMES/$1$CONF_SSHDIR

    cd $COLL_SSHDIR

    # add our public key to accept self login
    install -m 644 id_dsa.pub authorized_keys

    # add localhost public key to accept blind connection
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

    # /var/cache/mediatex/mdtx/home/$1/.ssh/config
    SSH_CONFIG=$HOMES/$1$CONF_SSHDIR$CONF_SSHCONF

    echo -e "Host *\n" > $SSH_CONFIG
    echo -e "\tVerifyHostKeyDNS yes\n" >> $SSH_CONFIG
    echo -e "\tHostKeyAlgorithms ssh-rsa,ssh-dss\n\n" >> $SSH_CONFIG
    echo -e "Host $2:\n\tPort $3\n" >> $SSH_CONFIG
    chown $1:$1 $SSH_CONFIG
}

# enable ssh login into chroot
# $1: yes or no
function SSH_chroot_login()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"

    # protect stars from shell expension
    PATTERN="$MDTX-*"
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

