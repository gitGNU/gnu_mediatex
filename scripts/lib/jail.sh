#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: jail.sh,v 1.3 2015/06/03 14:03:26 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the chroot jail.
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

### very helpfull !
# tail /var/log/auth.log
###

# includes
MDTX_SH_JAIL=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
#[ ! -e /usr/share/initramfs-tools/hook-functions ] || 
#source /usr/share/initramfs-tools/hook-functions

# add a binary and its dependencies into the jail
# $1: binary to add 
function JAIL_add_binary()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    #if [ -e /usr/share/initramfs-tools/hook-functions ]; then
	#copy_exec $i
    #else
        # iggy ld-linux* file as it is not shared one
	FILES="$(ldd $1 | awk '{ print $3 }' |egrep -v ^'\(')"
	for i in $1 $FILES; do
	    d="$(dirname $i)"
	    [ $UNIT_TEST_RUNNING -eq 1 ] || Debug "copying: $i"
	    [ -d $JAIL$d ] || mkdir -p $JAIL$d
	    [ -f $JAIL$i ] || /bin/cp $i $JAIL$d
	done
	
        # copy /lib/ld-linux* or /lib64/ld-linux* to $BASE/$sldlsubdir
        # get ld-linux full file location 
	sldl="$(ldd $1 | grep 'ld-linux' | awk '{ print $1}')"
	d="$(dirname $sldl)"
	[ $UNIT_TEST_RUNNING -eq 1 ] || Debug "copying: $sldl"
	[ -d $JAIL$sldl ] || mkdir -p $JAIL$d
	[ -f $JAIL$sldl ] || /bin/cp $sldl $JAIL$d
    #fi
}

## API

# build the chroot jail
# Note: the chroot jail MUST be owned and only be readable by root
function JAIL_build()
{
    Debug "$FUNCNAME:" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"
    JAIL=$CACHEDIR/$MDTX/jail
    DESTDIR=$JAIL # used by copy_exec
    BINARIES="/bin/ls /bin/bash /usr/bin/id /usr/bin/scp /usr/bin/cvs"

    mkdir -p $JAIL/{bin,dev,etc,tmp,usr/bin}
    mkdir -p $JAIL/lib/i386-linux-gnu/i686/cmo
    mkdir -p $JAIL/{var/cache,var/lib/cvsroot,var/tmp}

    for i in $BINARIES; do JAIL_add_binary $i; done
    cp /etc/ld.so.cache $JAIL/etc
    cp /etc/ld.so.conf $JAIL/etc
 
    # must be (not added yet): libnss_file.so
    FILES=$(find /lib -name "libnss_files.so*")
    for i in $FILES; do
    	d="$(dirname $i)"
    	[ $UNIT_TEST_RUNNING -eq 1 ] || Debug "copying: $i"
    	[ -d $JAIL$d ] || mkdir -p $JAIL$d
    	[ -f $JAIL$i ] || /bin/cp $i $JAIL$d
    done
    
    # needed by scp
    [ -e $JAIL/dev/null ] || mknod -m 666 $JAIL/dev/null c 1 3

    # users and groups
    for FILE in /etc/passwd /etc/group; do
	rm -f ${JAIL}${FILE}
	for MEMBER in root www-data $MDTX ${MDTX}_md; do
	    grep "^$MEMBER:" $FILE | cat >> ${JAIL}${FILE}
	done
    done

    # needed by cvs
    chmod 755 $JAIL/var/lib
    chmod 777 $JAIL/tmp
    chmod 777 $JAIL/var/tmp

    # still not needed
    #cp /etc/nsswitch.conf $JAIL/etc
    #cp /etc/hosts $JAIL/etc
    #mount -t proc proc jail/proc
    #mount -t sysfs sysfs jail/sys
    #mount -t devpts devpts jail/dev/pts
}

# del a user from the jail
# $1: user to del
function JAIL_del_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    [ $# -eq 1 ] || Error "expect 1 parameter"
    JAIL=$CACHEDIR/$MDTX/jail

    # users and groups
    for FILE in /etc/passwd /etc/group; do
	for MEMBER in $1 ${MDTX}_md; do
	    sed -i -e "/^$1:/ d" ${JAIL}${FILE}
	done
    done
}

# add a user to the jail
# $1: user to add 
function JAIL_add_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    [ $# -eq 1 ] || Error "expect 1 parameter"
    JAIL=$CACHEDIR/$MDTX/jail

    # users and groups
    for FILE in /etc/passwd /etc/group; do
	for MEMBER in $1 ${MDTX}_md; do
	    sed -i -e "/^$1:/ d" ${JAIL}${FILE}
	    grep "^$1:" $FILE | cat >> ${JAIL}${FILE}
	done
    done
}

function JAIL_bind()
{
    Debug $FUNCNAME 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    JAIL=$CACHEDIR/$MDTX/jail

    grep -q $JAIL/var/cache /etc/mtab || 
    mount --bind $CACHEDIR/$MDTX/cache $JAIL/var/cache
    grep -q $JAIL/var/cache /etc/mtab || 
    Error "Cannot bind $JAIL/var/cache"

    grep -q $JAIL/var/lib/cvsroot /etc/mtab || 
    mount --bind $STATEDIR/$MDTX $JAIL/var/lib/cvsroot
    grep -q $JAIL/var/lib/cvsroot /etc/mtab || 
    Error "Cannot bind $JAIL/var/lib/cvsroot"
}

# $1: user to unbind
function JAIL_unbind()
{
    Debug $FUNCNAME 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    JAIL=$CACHEDIR/$MDTX/jail

    [ $(grep -c $JAIL/var/cache /etc/mtab) -eq 0 ] ||
    umount $JAIL/var/cache
    [ $(grep -c $JAIL/var/cache /etc/mtab) -eq 0 ] ||
    Error "Cannot unbind $JAIL/var/cache"

    [ $(grep -c $JAIL/var/lib/cvsroot /etc/mtab) -eq 0 ] ||
    umount $JAIL/var/lib/cvsroot
    [ $(grep -c $JAIL/var/lib/cvsroot /etc/mtab) -eq 0 ] || 
    Error "Cannot unbind $JAIL/var/lib/cvsroot"
}

# unitary tests
if UNIT_TEST_start "jail"; then
    [ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh
    [ ! -z $MDTX_SH_SSH ]   || source $libdir/ssh.sh
    [ ! -z $MDTX_SH_CVS ]   || source $libdir/cvs.sh

    MDTX="ut4-mdtx"
    COLL="hello"
    USER="$MDTX-$COLL"

    # cleanup if previous test has failed
    JAIL_unbind
    USERS_mdtx_remove_user
    USERS_coll_remove_user $USER
    USERS_mdtx_disease
    
    # cf init.sh
    USERS_root_populate
    USERS_mdtx_create_user
    CVS_mdtx_setup
    SSH_chroot_login yes
    JAIL_build

    ## local check
    Info "chroot $JAIL ls"
    chroot $JAIL ls

    # cf new.sh
    USERS_coll_create_user $USER
    CVS_coll_import $USER
    SSH_build_key $USER
    SSH_bootstrapKeys $USER
    SSH_configure_client $USER "localhost" "22"
    JAIL_add_user $USER	

    ## remote login checks
    QUERY="ssh -o PasswordAuthentication=no ${USER}@localhost ls"
    Info "su LABEL -c \"$QUERY\""
    su $USER -c "$QUERY" || Error "Cannot connect via ssh"
    
    ## cf init.d 
    JAIL_bind
    
    ## scp and cvs checks
    touch $CACHEDIR/$MDTX/cache/$USER/hello.txt
    chown $USER.$USER $CACHEDIR/$MDTX/cache/$USER/hello.txt

    CACHE="$CACHEDIR/$MDTX/tmp/$USER"
    QUERY="scp ${USER}@localhost:/var/cache/$USER/hello.txt $CACHE"

    Info "su USER -c \"$QUERY\""
    su $USER -c "$QUERY" || Error "Cannot copy via ssh"
    CVS_coll_checkout $USER $MDTX $COLL "localhost"
    
    #echo "results :"
    #cd $UNIT_TEST_ROOTDIR
    #find . -ls |
    #awk '{ printf("%14s %14s %s %s\n",$5,$6,$3,$11) }' |
    #sort -k4
    #cd - >/dev/null

    # cf init.d
    JAIL_unbind
    JAIL_del_user $USER

    # cf free.sh & remove.sh
    SSH_chroot_login no
    USERS_coll_disease $USER
    USERS_coll_remove_user $USER
    USERS_mdtx_remove_user

    Info "success"
    UNIT_TEST_stop "jail"
fi
