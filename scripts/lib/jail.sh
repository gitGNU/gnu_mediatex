#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the chroot jail.
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

### helpfull !
# - tail /var/log/auth.log
# - strace -f -p $SSHD_PID
###

# includes
MDTX_SH_JAIL=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# copy a file into the jail
function JAIL_cp()
{
    #Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    DIR="$(dirname $1)"
    [ $UNIT_TEST_RUNNING -eq 1 ] || Debug "copying: $1"
    [ -d $JAIL$DIR ] || mkdir -p $JAIL$DIR
    [ -f $JAIL$1 ] || /bin/cp $1 $JAIL$DIR
}

# add a binary and its lib's dependencies into the jail
# $1: binary to add 
function JAIL_add_binary()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    FILES="$(ldd $1 | awk '{ print $3 }' | egrep -v ^'\(')"
    for FILE in $1 $FILES; do
	JAIL_cp $FILE
    done
}

# add a library into the jail
# $1: library to add
function JAIL_add_library()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # get all related so files and symlinks
    FILES=$(find /lib \( -type f -or -type l \) -name ${1}*)
    if [ -d /lib64 ]; then
	FILES64=$(find /lib64 \( -type f -or -type l \)  -name ${1}*)
	FILES="$FILES $FILES64"
    fi

    for FILE in $FILES; do
	JAIL_cp $FILE
    done
}

## API

# (re-)build the chroot jail
# Note: the chroot jail MUST be owned and only be readable by root
#  reference code should be here :
#  /usr/share/initramfs-tools/hook-functions::copy_exec()
function JAIL_build()
{
    Debug "$FUNCNAME:" 2
    [ $(id -u) -eq 0 ] || Error $0 $LINENO "need to be root"

    # /var/cache/mediatex/mdtx/jail
    DESTDIR=$JAIL # used by copy_exec
    BINARIES="/bin/ls /bin/bash /usr/bin/id /usr/bin/scp"
    BINARIES="$BINARIES /usr/bin/git"
    BINARIES="$BINARIES /usr/bin/git-upload-pack /usr/bin/git-receive-pack"
    FILES="/etc/ld.so.cache /etc/ld.so.conf"

    # librairies not automatically detected,
    #  found using `strace -f -p $SSHD_PID`
    LIBRARIES="ld-linux libnss_files" # libnss_compat libnsl libnss_nis"

    rm -fr $JAIL
    USERS_install $JAIL "${_VAR_CACHE_M_MDTX_JAIL[@]}"
    mkdir -p $JAIL/{bin,dev,etc,tmp,usr/bin} #,proc,sys,dev/pts}
    mkdir -p $JAIL/{var/cache,var/lib/gitbare,var/tmp}

    for i in $BINARIES; do
	JAIL_add_binary $i;
    done
    for i in $LIBRARIES; do
	JAIL_add_library $i
    done
    for i in $FILES; do
	JAIL_cp $i;
    done

    # needed by scp
    [ -e $JAIL/dev/null ] || mknod -m 666 $JAIL/dev/null c 1 3

    # users and groups: merge files if they already exist
    for FILE in /etc/passwd /etc/group; do
	touch $JAIL$FILE
	for MEMBER in root www-data $MDTX ${MDTX}_md; do
	    if ! grep -q "^$MEMBER:" $JAIL$FILE; then
		grep "^$MEMBER:" $FILE | cat >> ${JAIL}${FILE}
	    fi
	done
    done

    # needed by git
    chmod 755 $JAIL/var/lib
    chmod 777 $JAIL/tmp
    chmod 777 $JAIL/var/tmp

    # not needed (but maybe later)
    #cp /etc/nsswitch.conf $JAIL/etc
    #cp /etc/hosts $JAIL/etc
}

# del a user from the jail
# $1: user to del
function JAIL_del_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # users and groups
    for FILE in /etc/passwd /etc/group; do
	    sed -i -e "/^$1:/ d" ${JAIL}${FILE}
    done
}

# add a user to the jail
# $1: user to add 
function JAIL_add_user()
{
    Debug "$FUNCNAME: $1" 2
    [ $(id -u) -eq 0 ] || Error "need to be root"
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # users and groups
    for FILE in /etc/passwd /etc/group; do
	sed -i -e "/^$1:/ d" ${JAIL}${FILE}
	grep "^$1:" $FILE | cat >> ${JAIL}${FILE}
    done
}

function JAIL_bind()
{
    Debug $FUNCNAME 2
    [ $(id -u) -eq 0 ] || Error "need to be root"

    grep -q $JAIL/var/cache /etc/mtab ||
	mount --bind $MDTXHOME/cache $JAIL/var/cache
    grep -q $JAIL/var/cache /etc/mtab ||
	Error "Cannot bind $JAIL/var/cache"

    grep -q $JAIL/var/lib/gitbare /etc/mtab ||
	mount --bind $GITBARE $JAIL/var/lib/gitbare
    grep -q $JAIL/var/lib/gitbare /etc/mtab ||
	Error "Cannot bind $JAIL/var/lib/gitbare"

    # remove "Could not chdir to home directory" warning.
    #  Theses files are not backuped because they are written into
    #  a binded directory.
    mkdir -p $JAIL$MDTXHOME
    rm -f $JAIL$HOMES
    ln -s /var/cache $JAIL$HOMES
    
    # not needed (but maybe later)

    # grep -q $JAIL/proc /etc/mtab ||
    # 	mount -t proc proc $JAIL/proc
    # grep -q $JAIL/proc /etc/mtab ||
    # 	Error "Cannot bind $JAIL/proc"

    # grep -q $JAIL/sys /etc/mtab ||
    # 	mount -t sysfs sysfs $JAIL/sys
    # grep -q $JAIL/sys /etc/mtab ||
    # 	Error "Cannot bind $JAIL/sys"

    # grep -q $JAIL/dev/pts /etc/mtab ||
    # 	mount -t devpts devpts $JAIL/dev/pts
    # grep -q $JAIL/dev/pts /etc/mtab ||
    # 	Error "Cannot bind $JAIL/dev/pts"
}

# $1: user to unbind
function JAIL_unbind()
{
    Debug $FUNCNAME 2
    [ $(id -u) -eq 0 ] || Error "need to be root"

    [ $(grep -c $JAIL/var/cache /etc/mtab) -eq 0 ] ||
	umount $JAIL/var/cache
    [ $(grep -c $JAIL/var/cache /etc/mtab) -eq 0 ] ||
	Error "Cannot unbind $JAIL/var/cache"

    [ $(grep -c $JAIL/var/lib/gitbare /etc/mtab) -eq 0 ] ||
	umount $JAIL/var/lib/gitbare
    [ $(grep -c $JAIL/var/lib/gitbare /etc/mtab) -eq 0 ] || 
	Error "Cannot unbind $JAIL/var/lib/gitbare"

    # not needed (but maybe later)

    # [ $(grep -c $JAIL/proc /etc/mtab) -eq 0 ] ||
    # 	umount $JAIL/proc
    # [ $(grep -c $JAIL/proc /etc/mtab) -eq 0 ] ||
    # 	Error "Cannot unbind $JAIL/proc"

    # [ $(grep -c $JAIL/sys /etc/mtab) -eq 0 ] ||
    # 	umount $JAIL/sys
    # [ $(grep -c $JAIL/sys /etc/mtab) -eq 0 ] ||
    # 	Error "Cannot unbind $JAIL/sys"

    # [ $(grep -c $JAIL/dev/pts /etc/mtab) -eq 0 ] ||
    # 	umount $JAIL/dev/pts
    # [ $(grep -c $JAIL/dev/pts /etc/mtab) -eq 0 ] ||
    # 	Error "Cannot unbind $JAIL/dev/pts"
}
