#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: cvs.sh,v 1.3 2014/11/13 16:36:12 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx cvsroot and working copy cvs 
# * repositories.
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

# includes
MDTX_SH_CVS=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# this function initialise a CVS repository
function CVS_init_cvsroot()
{
    Debug $FUNCNAME 2
    CVSROOT=$STATEDIR/$MDTX

    if [ -d $CVSROOT/CVSROOT/history ]; then
	Warning "re-use already existing mdtx's cvsroot"
    else
	su $MDTX -c "cvs -d $CVSROOT init" ||
	Error "fails to initialize cvsroot"
    fi

    # set permissions
    chmod 777 $CVSROOT/CVSROOT # TO REMOVE ?
}

# this function import the mdtx module
function CVS_mdtx_import()
{
    Debug "$FUNCNAME $1" 2
    CVSROOT=$STATEDIR/$MDTX
    EXAMPLE=$DATADIR/examples
    CVS=$CACHEDIR/$MDTX/cvs/$MDTX

    if [ -f $CVSROOT/$MDTX/$MDTX.conf,v ]; then
	Warning "re-use already imported mdtx module"
    else 
	cd $CVS

	# add files
	install -o $MDTX -g $MDTX -m 660 $EXAMPLE$MEDIATEX.conf $MDTX.conf
	install -o $MDTX -g $MDTX -m 660 $EXAMPLE/supports.txt .

	# import them
	QUERY="CVSUMASK=027"
	QUERY="$QUERY cvs -d $CVSROOT import -m 'Mediatex initial setup'"
	QUERY="$QUERY $MDTX $MDTX v1"
	Info "$QUERY"
	su $MDTX -c "$QUERY" 2>&1 | sort ||
	Error "error importing $1's cvs project (never raised)"
	cd - >/dev/null
	rm -fr $CVS/*
    fi
}

# this function import a collection module
# $1: user (MDTX-COLL)
function CVS_coll_import()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    CVSROOT=$STATEDIR/$MDTX
    EXAMPLE=$DATADIR/examples
    CVS=$CACHEDIR/$MDTX/cvs/$1

    # create CVSROOT dir (or re-use it)
    install -o $1 -g $1 -m 2750 -d $CVSROOT/$1

    if [ -f $CVSROOT/$1/logo.png,v ]; then
	Warning "re-use already imported collection module"
	chown -R $1.$1 $CVSROOT/$1
    else 
	cd $CVS

	# add files
	install -o $USER -g $USER -m 770 -d apache2
	install -o $USER -g $USER -m 770 -d icons
	install -o $USER -g $USER -m 660 $EXAMPLE/floppy-icon.png icons
	install -o $USER -g $USER -m 660 $EXAMPLE/home.htaccess apache2
	install -o $USER -g $USER -m 660 $EXAMPLE/htgroup apache2
	sed apache2/htgroup -i -e "s!MDTX!$MDTX!"
	for f in logo.png mediatex.css \
	    catalog00.txt extract00.txt servers.txt 
	do
	    install -o $USER -g $USER -m 660 $EXAMPLE/$f .
	done

	for t in home index cache score cgi; do
	    install -o $USER -g $USER -m 660 $EXAMPLE/$t.htaccess apache2

	    # adapt them
	    if [ $t == cgi ]; then continue; fi
	    sed apache2/$t.htaccess -i -e "s!MEDIATEX!$MEDIATEX!"
	    sed apache2/$t.htaccess -i -e "s!MDTX-COLL!$1!"
	    sed apache2/$t.htaccess -i -e "s!MDTX!$MDTX!"
	    sed apache2/$t.htaccess -i -e "s!ETCDIR!$ETCDIR!"
	done

	cat > .cvsignore <<EOF
*NN.txt
EOF
	chown $USER:$USER .cvsignore

	# create mdtx password (bypass by unit tests)
	[ $UNIT_TEST_RUNNING -eq 1 ] ||
	htdigest -c apache2/htpasswd $1 $MDTX

	# import them
	QUERY="CVSUMASK=007"
	QUERY="$QUERY cvs -d $CVSROOT import -m 'Mediatex exemple files'"
	QUERY="$QUERY $USER $USER v1"
	Info "$QUERY"
	su $USER -c "cd $CVS && $QUERY" 2>&1 | 
	sort || Error "error importing $1's cvs project (never raised)"
	cd - >/dev/null
	rm -fr $CVS/*
 	rm -f $CVS/.cvsignore
    fi
}

## Wrapper fonctions above

# checkout the mdtx module
function CVS_mdtx_checkout()
{
    Debug "$FUNCNAME:" 2
    CVSROOT=$STATEDIR/$MDTX
    CVS=$CACHEDIR/$MDTX/cvs

    if [ -d $CVS/$MDTX/CVS ]; then
	Warning "re-use already checkout cvs module: $MDTX"
    else
	cd $CVS
	UMASK=$(umask -p)
	umask 0007

	QUERY="cvs -d $CVSROOT co $MDTX" 
	Info "su MDTX -c \"$QUERY\""
	su $MDTX -c "$QUERY" | sort ||
	Error "cannot checkout module: $MDTX (never raised)"

	eval $UMASK
	cd - > /dev/null
    fi
}

# checkout a collection module
# $1: user
# $2: remote mdtx user
# $3: collection label
# $4: host
function CVS_coll_checkout()
{
    Debug "$FUNCNAME: $1 $2-$3@$4" 2
    [ $# -eq 4 ] || Error "expect 4 parameter"
    CVS=$CACHEDIR/$MDTX/cvs
    MODULE="$2-$3"
    CVSROOT=":ext:${2}-${3}@${4}:/var/lib/cvsroot"

#    if [ -d $CVS/$1/CVS ]; then
#	Warning "re-use already checkout cvs module: $1"
#    else
	cd $CVS || Error "cannot cd to cvs working directory: $CVS"
	[ "$MODULE" = "$1" ] ||  mv $1 $MODULE

	# force checkout
	rm -fr $CVS/$MODULE/*
	rm -f $CVS/$MODULE/.cvsignore

	UMASK=$(umask -p)
	umask 0007

	QUERY="cvs -d $CVSROOT co $MODULE" 
	Info "su USER -c \"$QUERY\""
	su $USER -c "$QUERY" 2>&1 ||
	Error "cannot checkout module: $MODULE (never raised)"

	eval $UMASK
	[ "$MODULE" = "$1" ] || mv $MODULE $1
	cd - > /dev/null || true
#    fi
}

# this function update a module
# $1: cvs module to update
function CVS_update()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    CVS=$CACHEDIR/$MDTX/cvs/$1

    cd $CVS ||
    Error "cannot cd to cvs working directory: $CVS"
    UMASK=$(umask -p)
    umask 0007

    QUERY="cvs update -d"
    Info "$QUERY"
    eval $QUERY 2>&1 ||
    Error "cannot update cvs working directory: $CVS"

    # fix buggy cvs remote update
    chmod -R g+w *

    eval $UMASK
    cd - > /dev/null 2>&1 || true
}

# this function commit change into a module
# $1: cvs module to commit
# $2: message for this commit
function CVS_commit()
{
    Debug "$FUNCNAME: $1 $2" 2
    [ $# -eq 2 ] || Error "expect 2 parameters"
    CVS=$CACHEDIR/$MDTX/cvs/$1
    QUERY2="cvs commit -m \"$2\""

    cd $CVS ||
    Error "cannot cd to cvs working directory: $CVS"
    UMASK=$(umask -p)
    umask 0007

    # add meta-data parts to collections modules
    if [ "$1" != "$MDTX" ]; then
	QUERY1="cvs add *[0-9][0-9].txt icons/*.*"
	Info "$QUERY1"
	eval $QUERY1 2>&1 || /bin/true
    fi

    # su to mdtx user when call from init.sh
    if [ $(id -u) -eq 0 ]; then
	QUERY2="su $MDTX -c '$QUERY'"
    fi

    Info "$QUERY2"
    eval $QUERY2 2>&1 ||
    Error "cannot commit cvs working directory: $CVS"

    eval $UMASK
    cd - > /dev/null 2>&1 || true
}

# this function setup the mdtx module
function CVS_mdtx_setup()
{
    Debug "$FUNCNAME:" 2

    CVS_init_cvsroot
    CVS_mdtx_import
    CVS_mdtx_checkout
    CVS_update $MDTX
    CVS_commit $MDTX "$(basename $0)"
}

# unitary tests
if UNIT_TEST_start "cvs"; then
    [ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh
    [ ! -z $MDTX_SH_SSH ]   || source $libdir/ssh.sh

    MDTX="ut2-mdtx"
    COLL="hello"
    USER="$MDTX-$COLL"

    # cleanup if previous test has failed
    USERS_coll_remove_user $USER
    USERS_mdtx_remove_user

    # cf init.sh
    USERS_root_populate
    USERS_mdtx_create_user
    CVS_mdtx_setup
    
    # test mdtx module
    cd $CACHEDIR/$MDTX/cvs/$MDTX
    echo -e "\n# test\n" >> $MDTX.conf
    su $MDTX -c "cvs commit -m \"unit-test\"" 2>&1 | sort
    su $MDTX -c "cvs update -d" 2>&1 | sort
    su $MDTX -c "cvs log $MDTX.conf"
    cd - >/dev/null
    
    # cf new.sh
    USERS_coll_create_user $USER
    CVS_coll_import $USER
    SSH_build_key $USER
    SSH_bootstrapKeys $USER
    SSH_configure_client $USER "localhost" 22

    # test ssh
    QUERY="ssh -o PasswordAuthentication=no ${USER}@localhost ls"
    Info "su USER -c \"$QUERY\""
    su $USER -c "$QUERY" || Error "Cannot connect via ssh"

    # test collection module
    cd $CACHEDIR/$MDTX/cvs
    su $USER -c "cvs -d :ext:$USER@localhost:$STATEDIR/$MDTX co $USER" | 
    sort
    cd - >/dev/null
    cd $CACHEDIR/$MDTX/cvs/$USER
    echo -e "\n# test\n" >> $CACHEDIR/$MDTX/cvs/$USER/catalog00.txt
    su $USER -c "cvs commit -m \"unit-test\"" 2>&1 | sort
    su $USER -c "cvs update -d" 2>&1 | sort
    su $USER -c "cvs log catalog00.txt"
    cd - >/dev/null
   
    # cf free.sh & remove.sh
    USERS_coll_remove_user $USER
    USERS_mdtx_remove_user

    Info "success" 
    UNIT_TEST_stop "cvs"
fi
