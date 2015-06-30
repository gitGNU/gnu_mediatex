#!/bin/bash
#=======================================================================
# * Version: $Id: cvs.sh,v 1.5 2015/06/30 17:37:23 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx cvsroot and working copy cvs 
# * repositories.
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

# includes
MDTX_SH_CVS=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# this function initialise a CVS repository
function CVS_init_cvsroot()
{
    Debug $FUNCNAME 2
    # /var/lib/mediatex/mdtx/

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

    # /var/cache/mediatex/mdtx/cvs/mdtx
    CVS=$CVSCLT/$MDTX

    # /var/lib/mediatex/mdtx/mdtx/mdtx.conf,v
    if [ -f $CVSROOT/$MDTX/$MDTX$CONF_CONFFILE,v ]; then
	Warning "re-use already imported mdtx module"
    else 
	cd $CVS

	# add files from /usr/share/mediatex/misc/
	install -o $MDTX -g $MDTX -m 660 $MISC$MEDIATEX$CONF_CONFFILE \
	    $MDTX$CONF_CONFFILE
	install -o $MDTX -g $MDTX -m 660 $MISC$CONF_SUPPFILE .

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
# $1: collection user (MDTX-COLL)
function CVS_coll_import()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # /var/cache/mediatex/mdtx/cvs/mdtx-coll
    CVS=$CVSCLT/$1

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
	install -o $USER -g $USER -m 660 $MISC/floppy-icon.png icons
	install -o $USER -g $USER -m 660 $MISC/home.htaccess apache2
	install -o $USER -g $USER -m 660 $MISC/htgroup apache2

	sed apache2/htgroup -i -e "s!MDTX!$MDTX!"

	for f in logo.png mediatex.css ${CONF_CATHFILE}00.txt \
	    ${CONF_EXTRFILE}00.txt ${CONF_SERVFILE}.txt 
	do
	    install -o $USER -g $USER -m 660 $MISC/$f .
	done

	for t in home index cache score cgi; do
	    install -o $USER -g $USER -m 660 $MISC/$t.htaccess apache2

	    # adapt them
	    if [ $t == cgi ]; then continue; fi
	    sed apache2/$t.htaccess -i \
		-e "s!MEDIATEX!$MEDIATEX!" \
		-e "s!MDTX-COLL!$1!" \
		-e "s!MDTX!$MDTX!" \
		-e "s!ETCDIR!$ETCDIR!"
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
	sort || Error "error importing $1's cvs project"
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

    if [ -d $CVSCLT/$MDTX/CVS ]; then
	Warning "re-use already checkout cvs module: $MDTX"
    else
	cd $CVSCLT
	UMASK=$(umask -p)
	umask 0007

	QUERY="cvs -d $CVSROOT co $MDTX" 
	Info "su MDTX -c \"$QUERY\""
	su $MDTX -c "$QUERY" | sort ||
	Error "cannot checkout module: $MDTX"

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

    EXT_MODULE="$2-$3"
    EXT_CVSROOT=":ext:${2}-${3}@${4}:/var/lib/cvsroot"

#    if [ -d $CVSCLT/$1/CVS ]; then
#	Warning "re-use already checkout cvs module: $1"
#    else
	cd $CVSCLT || Error "cannot cd to cvs working directory: $CVSCLT"
	[ "$EXT_MODULE" = "$1" ] || mv $1 $EXT_MODULE

	# force checkout
	rm -fr $CVSCLT/$EXT_MODULE/*
	rm -f $CVSCLT/$EXT_MODULE/.cvsignore

	UMASK=$(umask -p)
	umask 0007

	QUERY="cvs -d $EXT_CVSROOT co $EXT_MODULE" 
	Info "su USER -c \"$QUERY\""
	su $USER -c "$QUERY" 2>&1 ||
	Error "cannot checkout module: $EXT_MODULE (never raised)"

	eval $UMASK
	[ "$EXT_MODULE" = "$1" ] || mv $EXT_MODULE $1
	cd - > /dev/null || true
#    fi
}

# this function update a module
# $1: cvs module to update
function CVS_update()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # /var/cache/mediatex/mdtx/cvs/mdtx-coll
    cd $CVSCLT/$1 ||
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

    QUERY2="cvs commit -m \"$2\""

    # /var/cache/mediatex/mdtx/cvs/mdtx-coll
    cd $CVSCLT/$1 ||
    Error "cannot cd to cvs working directory: $CVSCLT/$1"
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
	#QUERY2="su $MDTX -c '$QUERY'"
	QUERY2="su $MDTX -c '$QUERY2'"
    fi

    Info "$QUERY2"
    eval $QUERY2 2>&1 ||
    Error "cannot commit cvs working directory: $CVSCLT/$1"

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
