#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx users and related home directories
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

# includes
MDTX_SH_USERS=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# get a random number from 0 to $1 on 2 digits
# $1: maximum
# $rand (out): result
function USERS_root_random()
{
    RAND=$(echo "$RANDOM*($1+1)/32768" | bc)
    RAND=$(printf "%02i\n" $RAND)
}

# install a directory
# $1: directory
# $2: user
# $3: group
# $4: perm
# $5: default acl
function USERS_install()
{
    Debug "$FUNCNAME: $*" 2
    USR=$2
    GRP=$3
    ACL=$5

    if [ "$USR" == "%s" ]; then
	USR=$MDTX-$COLL
    fi
    if [ "$GRP" == "%s" ]; then
	GRP=$MDTX-$COLL
    fi
    install -o $USR -g $GRP -m $4 -d $1

    if [ "$ACL" != "NO ACL" ]; then
	if [ ! -z "$ACL" ]; then
	    # default ACL: "u:%s:rwx g:%s:rwx [u:%s:r-x]"
	    ACL=$(echo $ACL | sed -e "s/%s/$MDTX/")
	    ACL=$(echo $ACL | sed -e "s/%s/$MDTX/")
	    ACL=$(echo $ACL | sed -e "s/%s/$MDTX-$COLL/")
	fi
	STR=""
	for VAL in $BASE_ACL $ACL; do
	    STR="$STR -m $VAL"
	done
	setfacl $STR $1
	setfacl -d $STR $1
    fi
}

# this function create the root directories
function USERS_root_populate()
{
    Debug "$FUNCNAME" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"

    USERS_install $STATEDIR "${_VAR_LIB_M[@]}"
    USERS_install $CACHEDIR "${_VAR_CACHE_M[@]}"
    USERS_install $ETCDIR "${_ETC_M[@]}"
    USERS_install $PIDDIR "${_VAR_RUN_M[@]}"
}

# this function remove the root directories
# it removes STATEDIR wich contains all the recovering data
function USERS_root_disease()
{
    Debug "$FUNCNAME" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    ALONE=1;
    
    # clean base directories if no more used
    for DIR in $STATEDIR $CACHEDIR $ETCDIR; do
	if [ -d "$DIR" ]; then
	    if [ $(ls $DIR | wc -l) -gt 0 ]; then
		ALONE=0
	    else
		rmdir $DIR
	    fi
	fi
    done
    if [ $ALONE -eq 1 -a -d "$PIDDIR" ]; then
	if [ $(ls $PIDDIR | wc -l) -gt 0 ]; then
	    rmdir $DIR
	fi
    fi
}

# this function create the mdtx directories
function USERS_mdtx_populate()
{
    Debug "$FUNCNAME" 2
    MDTX_HTML=$MDTXHOME$CONF_HTMLDIR

    # /var/lib/mediatex/mdtx
    USERS_install $GITBARE         "${_VAR_LIB_M_MDTX[@]}"
    USERS_install $GITBARE/$MDTX   "${_VAR_LIB_M_MDTX_MDTX[@]}"

    # /var/cache/mediatex/mdtx
    USERS_install $MDTXHOME  "${_VAR_CACHE_M_MDTX_HOME[@]}"
    USERS_install $MDTX_HTML "${_VAR_CACHE_M_MDTX_HTML[@]}"
    USERS_install $MD5SUMS   "${_VAR_CACHE_M_MDTX_MD5SUMS[@]}"
    USERS_install $CACHES    "${_VAR_CACHE_M_MDTX_CACHE[@]}"
    USERS_install $EXTRACT   "${_VAR_CACHE_M_MDTX_TMP[@]}"
    USERS_install $GITCLT    "${_VAR_CACHE_M_MDTX_GIT[@]}"
    USERS_install $MDTXGIT   "${_VAR_CACHE_M_MDTX_GIT_MDTX[@]}"

    # /etc/mediatex/mdtx.conf
    ln -sf $MDTXGIT/$MDTX$CONF_CONFFILE $ETCDIR/$MDTX$CONF_CONFFILE
}

# this function remove the server directories
# we do remove STATEDIR wich contains all the recovering data
function USERS_mdtx_disease()
{
    Debug "$FUNCNAME" 2

    # unconfigure related services
    USERS_mdtx_disable
    
    # /etc/mediatex/mdtx* (links)
    rm -fr $ETCDIR/${MDTX}*

    # /etc/apache2/conf.d/mediatex/mediatex-mdtx.conf
    rm -f $SYSCONFDIR/apache2/conf.d$MEDIATEX-$MDTX.conf

    # /var/cache/mediatex/mdtx
    rm -fr $MDTXHOME

    # /var/lib/mediatex/mdtx
    # do not destroy all meta-data repositories
    #rm -fr $GITBARE/$MDTX
}

# this function populate a collection
# $1: collection user
function USERS_coll_populate()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
  
    # /var/cache/mediatex/mdtx/*/mdtx-coll
    COLL_CACHE=$CACHES/$1
    COLL_EXTRACT=$EXTRACT/$1
    COLL_GIT=$GITCLT/$1
    COLL_HOME=$HOMES/$1
    COLL_SSH=$COLL_HOME$CONF_SSHDIR
    COLL_HTML=$COLL_HOME$CONF_HTMLDIR

    USERS_install $COLL_CACHE   "${_VAR_CACHE_M_MDTX_CACHE_COLL[@]}"
    USERS_install $COLL_EXTRACT "${_VAR_CACHE_M_MDTX_TMP_COLL[@]}"
    USERS_install $COLL_GIT     "${_VAR_CACHE_M_MDTX_GIT_COLL[@]}"
    USERS_install $COLL_HOME    "${_VAR_CACHE_M_MDTX_HOME_COLL[@]}"
    USERS_install $COLL_SSH     "${_VAR_CACHE_M_MDTX_HOME_COLL_SSH[@]}"
    USERS_install $COLL_HTML    "${_VAR_CACHE_M_MDTX_HOME_COLL_HTML[@]}"

    # link facilities
    for f in git cache; do
	rm -f $COLL_HOME/$f
	ln -sf ../../$f/$1 $COLL_HOME/$f 
    done
}

# this function remove a collection,
# but the collection may still recover from gitbare
# $1: collection user
function USERS_coll_disease()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # /var/cache/mediatex/mdtx/*/mdtx-coll
    COLL_CACHE=$CACHES/$1
    COLL_EXTRACT=$EXTRACT/$1
    COLL_GIT=$GITCLT/$1
    COLL_HOME=$HOMES/$1

    rm -fr $COLL_CACHE
    rm -fr $COLL_EXTRACT
    rm -fr $COLL_GIT
    rm -fr $COLL_HOME
    rm -f  $MD5SUMS/$1.md5

    # /etc/mediatex/mdtx-coll (link)
    rm -f $ETCDIR/$1
}

# this function configure related services
function USERS_mdtx_enable()
{
    Debug "$FUNCNAME" 2
    POSTFIX=""
    if [ "$MDTX" != "mdtx" ]; then
	POSTFIX="-$MDTX"
    fi
    CRON_FILE=$SYSCONFDIR/cron.d/mediatex$POSTFIX
    ROTATE_FILE=$SYSCONFDIR/logrotate.d/httpd-prerotate/mediatex$POSTFIX
    INIT_FILE=$SYSCONFDIR/init.d/mediatexd$POSTFIX

    # create the mdtx directories
    USERS_mdtx_populate
    
    # configure cron
    if [ ! -f $CRON_FILE ]; then
	install -o root -g root -m 640 $MISC/cron $CRON_FILE
	sed $CRON_FILE -i \
	    -e "s!DATADIR!$DATADIR!" \
	    -e "s!MDTX=mdtx!MDTX=$MDTX!"
	USERS_root_random 59
	sed $CRON_FILE -i -e "s!#M1!$RAND!"
	USERS_root_random 59
	sed $CRON_FILE -i -e "s!#M2!$RAND!"
	USERS_root_random 59
	sed $CRON_FILE -i -e "s!#M3!$RAND!"
	USERS_root_random 23
	sed $CRON_FILE -i -e "s!H1!$RAND!"
	USERS_root_random 23
	sed $CRON_FILE -i -e "s!H2!$RAND!"
    fi

    # configure logrotate on apache logs
    if [ ! -f $ROTATE_FILE ]; then
	cat >$ROTATE_FILE <<EOF
#!/bin/bash
MDTX=$MDTX $DATADIR/scripts/logrotate_apache.sh
EOF
	chmod +x $ROTATE_FILE
    fi

    # init script for other instances than mdtx
    if [ "$MDTX" != "mdtx" ]; then
	if [ ! -f $INIT_FILE ]; then
	    cp $SYSCONFDIR/init.d/mediatexd $INIT_FILE
	    sed -i $INIT_FILE \
		-e "s!\(# Provides:          mediatexd\)!\1$POSTFIX!" \
		-e "s!USER=mdtx!USER=$MDTX!"
	fi
	chmod +x $INIT_FILE
    fi
}

# this function unconfigure related services
function USERS_mdtx_disable()
{
    Debug "$FUNCNAME" 2
    POSTFIX=""
    if [ "$MDTX" != "mdtx" ]; then
	POSTFIX="-$MDTX"
    fi
    CRON_FILE=$SYSCONFDIR/cron.d/mediatex$POSTFIX
    ROTATE_FILE=$SYSCONFDIR/logrotate.d/httpd-prerotate/mediatex$POSTFIX
    INIT_FILE=$SYSCONFDIR/init.d/mediatexd$POSTFIX
    
    # unconfigure cron
    rm -f $CRON_FILE

    # unconfigure logrotate on apache logs
    rm -f $ROTATE_FILE

    # remove init script for other instances than mdtx
    if [ "$MDTX" != "mdtx" ]; then
	rm -f $INIT_FILE
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

# this function del a user from a group
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

    # home directory is /var/cache/mediatex/mdtx
    USERS_create_user $MDTX $MDTXHOME
    USERS_add_to_group $MDTX cdrom
    USERS_mdtx_enable
}

# this function remove the mdtx user
function USERS_mdtx_remove_user()
{
    Debug "$FUNCNAME: $1" 2

    USERS_remove_user $MDTX
    USERS_mdtx_disease
}

# this function create a coll user
# $1: collection user
function USERS_coll_create_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # /var/cache/mediatex/mdtx/home/mdtx-coll
    COLL_HOME=$HOMES/$1

    USERS_create_user $1 $COLL_HOME
    USERS_coll_populate $1
}

# this function remove a coll user
# $1: collection user
function USERS_coll_remove_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    USERS_remove_user $1
    USERS_coll_disease $1
}
