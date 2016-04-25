#!/bin/bash
#=======================================================================
# * Version: $Id: cvs.sh,v 1.8 2015/10/02 18:02:21 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx cvsroot and working copy cvs 
# * repositories.
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
MDTX_SH_GIT=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/scripts/lib
[ ! -z $MDTX_SH_LOG ] || source $libdir/log.sh
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# this function set the git configuration into its repository
# $1: git module to upgrade (MDTX or MDTX-COLL)
function GIT_upgrade()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    GIT=$CVSCLT/$1

    QUERIES=(\
	"git config color.diff auto"
	"git config color.status auto"
	"git config color.branch auto"
	"git config user.name 'nroche'"
	"git config user.email 'nroche@narval.tk'"
	"git config push.default simple"
    )

    # su to module user if (should not) call from init.sh
    if [ $(id -u) -eq 0 ]; then
	for I in $(seq 0 5); do
	    QUERIES[$I]="su $1 -c '${QUERIES[$I]}'"
	done
    else
	[ "$(whoami)" == $1 ] ||
	    Error "module $1 has to be updated by $1 user"	
    fi
    
    # /var/cache/mediatex/mdtx/git/$1
    cd $GIT || Error "cannot cd to git working directory: $GIT"
 
    for I in $(seq 0 5); do
	Info "${QUERIES[$I]}"
	eval ${QUERIES[$I]} ||
	    Error "cannot upgrade ($[$I+1]/6) git working directory: $GIT" 
    done
    
    cd - > /dev/null 2>&1 || true
}
    

# this function update a module
# $1: git module to update (MDTX or MDTX-COLL)
function GIT_update()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    GIT=$CVSCLT/$1
    QUERY1="git commit -a -m 'manual user edition'"
    QUERY2="git pull --no-edit"

    # su to module user if (should not) call from init.sh
    if [ $(id -u) -eq 0 ]; then
	QUERY1="su $1 -c \"$QUERY1\""
	QUERY2="su $1 -c \"$QUERY2\""
    else
	[ "$(whoami)" == $1 ] ||
	    Error "module $1 has to be updated by $1 user"	
    fi
    
    # /var/cache/mediatex/mdtx/git/$1
    cd $GIT || Error "cannot cd to git working directory: $GIT"

    if [ -n "$(git diff)" ]; then 
	Info "$QUERY1"
	eval $QUERY1 ||
	    Error "cannot update (1/2) git working directory: $GIT"    
    fi
	
    Info "$QUERY2"
    eval $QUERY2 ||
	Error "cannot update (2/2) git working directory: $GIT"    
    
    cd - > /dev/null 2>&1 || true
}

# this function commit change into a module
# $1: git module to commit (MDTX or MDTX-COLL)
# $2: message for this commit
function GIT_commit()
{
    Debug "$FUNCNAME: $1 $2" 2
    [ $# -eq 2 ] || Error "expect 2 parameters"
    GIT=$CVSCLT/$1
    QUERY1="git add *[0-9][0-9].txt icons/*.*"
    QUERY2="git commit -a -m \"$2\""
    QUERY3="git push"

    # /var/cache/mediatex/mdtx/cvs/mdtx-coll
    cd $GIT || Error "cannot cd to cvs working directory: $CVSCLT/$1"

    # su to module user when call from init.sh
    if [ $(id -u) -eq 0 ]; then
	QUERY1="su $1 -c '$QUERY1'"
	QUERY2="su $1 -c '$QUERY2'"
	QUERY3="su $1 -c '$QUERY3'"
    else
	[ "$(whoami)" == $1 ] ||
	    Error "module $1 has to be commited by $1 user"	
    fi
    
    # add meta-data parts to collections modules
    if [ "$1" != "$MDTX" ]; then
	Info "$QUERY1"
	eval $QUERY1 || Error "cannot commit (1/3) git module: $CVSCLT/$1"
    fi

    Info "$QUERY2"
    eval $QUERY2 || Error "cannot commit (2/3) git module: $CVSCLT/$1"

    Info "$QUERY3"
    eval $QUERY3 || Error "cannot commit (3/3) git module: $CVSCLT/$1"
    
    cd - > /dev/null 2>&1 || true
}

# this function initialise a GIT bare repository
# $1: git module initialise
function GIT_init_gitbare()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    QUERY="git init --bare $CVSROOT/$1"

    # /var/lib/mediatex/mdtx/$1/HEAD
    if [ -f $CVSROOT/$1/HEAD ]; then
	Warning "re-use already existing $1's gitbare"
    else
	Info "su $1 -c \"$QUERY\""
	su $1 -c "$QUERY" || Error "fails to initialize $1's gitbare"
    fi
}

# this function setup the mdtx module
function GIT_mdtx_import()
{
    Debug "$FUNCNAME" 2
    GIT=$CVSCLT/$MDTX
    QUERY1="git clone $CVSROOT/$MDTX $CVSCLT/$MDTX"
    QUERY2="git add -A"

    GIT_init_gitbare $MDTX
    
    # /var/cache/mediatex/mdtx/git/mdtx/mdtx.conf
    if [ -f $GIT/$MDTX$CONF_CONFFILE ]; then
	Warning "re-use already checkouted $MDTX module"
    else
	Info "su $MDTX -c \"$QUERY1\""
	su $MDTX -c "$QUERY1" || Error "cannot clone $MDTX module"
    fi

    # /var/cache/mediatex/mdtx/git/mdtx/mdtx.conf
    if [ -f $GIT/$MDTX$CONF_CONFFILE ]; then
	Warning "re-use already imported $MDTX module"
	chown -R $1.$1 $CVSROOT/$MDTX
    else
	# add files from /usr/share/mediatex/misc/
	install -o $MDTX -g $MDTX -m 660 \
		$MISC$MEDIATEX$CONF_CONFFILE \
		$GIT/$MDTX$CONF_CONFFILE
	install -o $MDTX -g $MDTX -m 640 \
		$MISC$CONF_SUPPFILE \
		$GIT/

       	Info "su $MDTX -c \"$QUERY2\""
	cd $GIT || Error "cannot cd to git working directory: $GIT"
	su $MDTX -c "$QUERY2" || Error "cannot add files to $MDTX module"
	cd - > /dev/null || true

	GIT_upgrade $MDTX
	GIT_commit $MDTX "Initial mdtx setup"
    fi
}

# this function setup a new collection module
# $1: collection user (MDTX-COLL)
function GIT_coll_import()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    GIT=$CVSCLT/$1
    QUERY1="git clone $CVSROOT/$1 $CVSCLT/$1"
    QUERY2="git add -A"

    GIT_init_gitbare $1
    
    # /var/cache/mediatex/mdtx/git/coll/logo
    if [ -f $GIT/logo ]; then
	Warning "re-use already checkouted $1 module"
    else
	Info "su $1 -c \"$QUERY1\""
	su $1 -c "$QUERY1" || Error "cannot checkout $1 module"
    fi
    
    if [ -d $CVSROOT/$1/logo ]; then
	Warning "re-use already imported collection module"
	chown -R $1.$1 $CVSROOT/$1
    else 
	cd $GIT || Error "cannot cd to git working directory: $GIT"
	
	# add files
	install -o $1 -g $1 -m 770 -d apache2
	install -o $1 -g $1 -m 770 -d icons
	install -o $1 -g $1 -m 660 $MISC/floppy-icon.png icons 
	install -o $1 -g $1 -m 660 $MISC/home.htaccess apache2
	install -o $1 -g $1 -m 660 $MISC/htgroup apache2

	sed apache2/htgroup -i -e "s!MDTX!$MDTX!"

	for f in logo mediatex.css ${CONF_CATHFILE}000.txt \
	    ${CONF_EXTRFILE}000.txt ${CONF_SERVFILE}.txt 
	do
	    install -o $1 -g $1 -m 660 $MISC/$f .
	done
	install -o $1 -g $1 -m 660 /dev/null readme.html

	for t in home index cache score cgi; do
	    install -o $1 -g $1 -m 660 $MISC/$t.htaccess apache2

	    # adapt them
	    if [ $t == cgi ]; then continue; fi
	    sed apache2/$t.htaccess -i \
		-e "s!MEDIATEX!$MEDIATEX!" \
		-e "s!MDTX-COLL!$1!" \
		-e "s!MDTX!$MDTX!" \
		-e "s!ETCDIR!$ETCDIR!"
	done

	cat > .cvsignore <<EOF
*NNN.txt
EOF
	chown $1:$1 .cvsignore

	# create mdtx password (bypass by unit tests)
	[ $UNIT_TEST_RUNNING -eq 1 ] ||
	    htdigest -c apache2/htpasswd $1 $MDTX

	# import them
       	Info "su $1 -c \"$QUERY2\""
	su $1 -c "$QUERY2" ||
	    Error "cannot add files to $1 module"
	cd - >/dev/null || true
	GIT_upgrade $1
	GIT_commit $1 "Initial collection setup"
    fi
}

# checkout a collection module
# $1: user
# $2: remote mdtx user
# $3: collection label
# $4: host
function GIT_coll_checkout()
{
    Debug "$FUNCNAME: $1 $2-$3@$4" 2
    [ $# -eq 4 ] || Error "expect 4 parameter"

    GIT=$CVSCLT/$1
    EXT_MODULE="$2-$3"
    QUERY="git clone ssh://$EXT_MODULE@$4:/var/lib/cvsroot/$1 $GIT"
       
    #EXT_CVSROOT=":ext:${2}-${3}@${4}:/var/lib/cvsroot"
    
    # force checkout using ssh
    rm -fr $CVSCLT/$EXT_MODULE $GIT
    USERS_install $GIT "${_VAR_LIB_M_MDTX_COLL[@]}"
    
    Info "su $1 -c \"$QUERY\""
    su $1 -c "$QUERY" || Error "cannot clone $EXT_MODULE module"
 
    [ "$EXT_MODULE" = "$1" ] || mv $CVSCLT/$EXT_MODULE $GIT
    cd - > /dev/null || true
    
    GIT_upgrade $1
}
