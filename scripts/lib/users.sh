#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: users.sh,v 1.3 2015/06/03 14:03:27 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx users and related home directory
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

# includes
MDTX_SH_USERS=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# get a random number from 0 to $1 on 2 digits
# $1: maximum
function USERS_root_random
{
    rand=$(echo "$RANDOM*($1+1)/32768" | bc)
    rand=$(printf "%02i\n" $rand)
}

# this function create the root directories
function USERS_root_populate()
{
    Debug "$FUNCNAME" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"

    install -o root -g root -m 755 -d $STATEDIR
    install -o root -g root -m 755 -d $CACHEDIR
    install -o root -g root -m 755 -d $ETCDIR
    install -o root -g root -m 755 -d $PIDDIR

    # configure cron
    install -o root -g root -m 640 $DATADIR/examples/mediatex_cron \
	$SYSCONFDIR/cron.d
    sed $SYSCONFDIR/cron.d/mediatex_cron -i -e "s!DATADIR!$DATADIR!"
    USERS_root_random 59
    sed $SYSCONFDIR/cron.d/mediatex_cron -i -e "s!#XX!$rand!"
    USERS_root_random 59
    sed $SYSCONFDIR/cron.d/mediatex_cron -i -e "s!#YY!$rand!"
    USERS_root_random 23
    sed $SYSCONFDIR/cron.d/mediatex_cron -i -e "s!ZZ!$rand!"
}

# this function remove the root directories
# it removes STATEDIR wich contains all the recovering data
function USERS_root_disease()
{
    Debug "$FUNCNAME" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    
    # clean base directories if no more used
    for DIR in $STATEDIR $CACHEDIR $ETCDIR $PIDDIR; do
	if [ -d "$DIR" ]; then
	    [ $(ls $DIR | wc -l) -gt 0 ] || rmdir $DIR
	fi
    done
}

# this function create the server directories
function USERS_mdtx_populate()
{
    Debug "$FUNCNAME" 2

    HOME=$CACHEDIR/$MDTX
    install -o root  -g root        -m 755  -d $HOME
    install -o root  -g root        -m 755  -d $HOME/jail
    install -o $MDTX -g $MDTX       -m 750  -d $HOME/md5sums
    install -o $MDTX -g ${MDTX}_md  -m 750  -d $HOME/cache
    install -o $MDTX -g ${MDTX}_md  -m 750  -d $HOME/tmp
    
    CVSROOT=$STATEDIR/$MDTX
    install -o $MDTX -g ${MDTX}_md  -m 750  -d $CVSROOT
    install -o $MDTX -g ${MDTX}_md  -m 2770 -d $CVSROOT/CVSROOT
    install -o $MDTX -g $MDTX       -m 2750 -d $CVSROOT/$MDTX

    install -o $MDTX -g ${MDTX}_md  -m 750  -d $HOME/cvs
    install -o $MDTX -g $MDTX       -m 2770 -d $HOME/cvs/$MDTX
    ln -sf  $HOME/cvs/$MDTX/$MDTX.conf $ETCDIR/$MDTX.conf

    install -o $MDTX -g $MDTX       -m 700  -d $HOME/.ssh
    install -o $MDTX -g $MDTX       -m 750  -d $HOME/public_html
}

# this function remove the server directories
# we do remove STATEDIR wich contains all the recovering data
function USERS_mdtx_disease()
{
    Debug "$FUNCNAME" 2
    rm -fr $ETCDIR/${MDTX}*
    rm -fr $CACHEDIR/$MDTX
    rm -f $SYSCONFDIR/apache2/conf.d$MEDIATEX-$MDTX.conf

    # purge was asked, so we destroy all data sources
    rm -fr $STATEDIR/$MDTX
}

# this function populate a collection
# $1: user
function USERS_coll_populate()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # new   
    CACHE=$CACHEDIR/$MDTX/cache/$1
    TOKEEP=$CACHE/toKeep
    TMP=$CACHEDIR/$MDTX/tmp/$1
    CVSROOT=$STATEDIR/$MDTX/$1
    CVS=$CACHEDIR/$MDTX/cvs/$1
    HOME=$CACHEDIR/$MDTX/home/$1

    install -o $MDTX -g $1    -m 2750 -d $CACHE
    install -o $MDTX -g $1    -m 2750 -d $TOKEEP
    install -o $MDTX -g $1    -m 2770 -d $TMP
    #install -o $1    -g $1    -m 2750 -d $CVSROOT (not on slave)
    install -o $MDTX -g $1    -m 2770 -d $CVS
    install -o $1    -g $MDTX -m 750  -d $HOME
    install -o $1    -g $1    -m 700  -d $HOME/.ssh
    install -o $MDTX -g $1    -m 2750 -d $HOME/public_html

    # link facilities
    for f in cvs cache; do
	rm -f $HOME/$f
	ln -sf ../../$f/$1 $HOME/$f 
    done
}

# this function remove a collection
# but the collection may be recover from cvsroot
# $1: user
function USERS_coll_disease()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # new   
    CACHE=$CACHEDIR/$MDTX/cache/$1
    TMP=$CACHEDIR/$MDTX/tmp/$1
    CVSROOT=$STATEDIR/$MDTX/$1
    CVS=$CACHEDIR/$MDTX/cvs/$1
    HOME=$CACHEDIR/$MDTX/home/$1

    rm -fr $CACHE
    rm -fr $TMP
    rm -fr $CVS
    rm -fr $HOME
    rm -f $ETCDIR/$1

    # do not remove data sources, but let the user do that
    #rm -fr $CVSROOT
}

# Create a group 
# $1: the group name
function USERS_create_group()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    [ $# -eq 1 ] || Error "expect 1 parameter"

    if [ -z "$(grep "^$1:" /etc/group)" ]; then
	/usr/sbin/addgroup \
	    --system \
	    --quiet $1 || 
	Error "addgroup $1 failed"
    else
	Info "$1 group already exists"
    fi
}

# this function disease a group
# $1: the group name
function USERS_disease_group()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"

    MEMBERS=$(grep "^$1:" /etc/group | cut -d : -f 4- | sed "s/,/ /g")
    if [ ! -z "MEMBERS" ]; then
	for _USERS in $MEMBERS; do
	    [ "$_USERS" != "$1" ] || continue
	    if [ -z "$(grep ^$_USERS: /etc/passwd)" ]; then
		Info "$_USERS user already removed"
	    else
		/usr/sbin/deluser $_USERS $1
	    fi
	done
    fi
}

# Remove a group
# $1: the group name
function USERS_remove_group()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # remove users from the user group
    USERS_disease_group $1

    # remove group
    if [ -z "$(grep ^$1: /etc/group)" ]; then
	Info "$1 group already removed"
    else
	/usr/sbin/delgroup $1 || Error "delgroup $1 failed"
    fi
}

# this function add a user to a group
# $1: the user name
# $2: the group name
function USERS_add_to_group()
{
    Debug "$FUNCNAME: $1 $2" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"

    MEMBERS=$(grep "^$2:" /etc/group | cut -d : -f 4-)
    if [ -z "$(echo ",$MEMBERS," | grep ",$1,")" ]; then
	/usr/sbin/adduser $1 $2
    else
	Info "$1 user already beyong to $2 group"
    fi
}

# this function add a user to a group
# $1: the user name
# $2: the group name
function USERS_del_from_group()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"

    MEMBERS=$(grep "^$2:" /etc/group | cut -d : -f 4-)
    if [ -z "$(echo ",$MEMBERS," | grep ",$1,")" ]; then
	Info "$1 user already removed from $2 group"
    else
	/usr/sbin/deluser $1 $2
    fi
}

# this function create a user (need /bin/bash in /etc/passwd for ssh)
# $1: the user name
# $2: home directory
function USERS_create_user()
{
    Debug "$FUNCNAME: $1 $2" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"
    [ $# -eq 2 ] || Error "expect 2 parameters"
    [ ! -d $2/$1 ] || Warning "reuse the existing $1's home"

    # non-sens here to use a fake /etc dir
    if [ -z "$(grep "^$1:" /etc/passwd)" ]; then
	/usr/sbin/adduser \
	    --system \
	    --group \
	    --home $2 \
	    --shell /bin/bash \
	    --quiet $1 || 
	Error "adduser $1 failed"
    else
	Info "$1 user already exists"
    fi
}

# this function remove a user
# $1: the user name (MDTX-COLL)
function USERS_remove_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"

    # remove groups
    USERS_disease_group $1

    # remove user
    if [ -z "$(grep ^$1: /etc/passwd)" ]; then
	Info "$1 user already removed"
    else
	if [ -z "$(ps --no-headers U $1)" ]; then
	    # --quiet
	    /usr/sbin/deluser $1 || 
	    Error $0 "deluser $1 failed"
	else
	    Warning "$1 user need to logout for account removal"
	fi
    fi
}

# this function create the mdtx user
function USERS_mdtx_create_user()
{
    Debug "$FUNCNAME: $1" 2
    HOME=$CACHEDIR/$MDTX

    USERS_create_user $MDTX $HOME
    USERS_add_to_group $MDTX cdrom
    USERS_add_to_group "www-data" $MDTX
    USERS_create_group ${MDTX}_md
    USERS_add_to_group $MDTX ${MDTX}_md
    USERS_add_to_group "www-data" ${MDTX}_md
    USERS_mdtx_populate
}

# this function remove the mdtx user
function USERS_mdtx_remove_user()
{
    Debug "$FUNCNAME: $1" 2

    USERS_remove_group ${MDTX}_md
    USERS_remove_user $MDTX
    USERS_mdtx_disease
}

# this function create a coll user
# $1: user
function USERS_coll_create_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    HOME=$CACHEDIR/$MDTX/home/$1

    USERS_create_user $1 $HOME
    USERS_add_to_group $1 ${MDTX}_md
    USERS_add_to_group "www-data" $1
    USERS_add_to_group $MDTX $1
    USERS_coll_populate $1

    # add users from the mdtx group
    MEMBERS=$(grep "^$MDTX:" /etc/group | cut -d : -f 4- | tr "," " ")
    for MEMBER in $MEMBERS; do
    	[ "$MEMBER" != "www-data" ] || continue
    	USERS_add_to_group $MEMBER $1
    done
}

# this function remove a coll user
# $1: user
function USERS_coll_remove_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    USERS_del_from_group $1 ${MDTX}_md
    USERS_remove_user $1
    USERS_coll_disease $1
}

# unitary tests
if UNIT_TEST_start "users"; then

    MDTX="ut1-mdtx"
    COLL="hello"
    USER="$MDTX-$COLL"

    # cleanup if previous test has failed
    USERS_mdtx_remove_user
    USERS_coll_remove_user $USER

    USERS_root_populate
    USERS_root_populate
    USERS_mdtx_create_user
    USERS_mdtx_create_user
    
    USERS_coll_create_user $USER
    USERS_coll_create_user $USER
    
    echo "home :"
    cd $UNIT_TEST_ROOTDIR
    find . -ls |
    awk '{ printf("%14s %14s %s %s\n",$5,$6,$3,$11) }' |
    sort -k4
    cd - >/dev/null
    
    USERS_coll_disease $USER
    USERS_coll_remove_user $USER
    USERS_coll_remove_user $USER
    USERS_mdtx_remove_user
    USERS_mdtx_remove_user
    
    echo "cleanup :"
    cd $UNIT_TEST_ROOTDIR
    find . -ls |
    awk '{ printf("%s %s\n",$3,$11) }' |
    sort -k4
    cd - >/dev/null

    Info "success"
    UNIT_TEST_stop "users"
fi
