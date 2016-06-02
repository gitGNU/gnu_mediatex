#!/bin/bash
#=======================================================================
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the mdtx gitbare and git working copy
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
# $2: author (hostname)
# $3: email  (fingerprint)
# $4: remote origin url (if provided)
function GIT_upgrade()
{
    Debug "$FUNCNAME: $*" 2
    [ $# -ge 3 ] || Error "expect at less 3 parameters"
    GIT=$GITCLT/$1

    local QUERIES=(\
	"git config color.diff auto"
	"git config color.status auto"
	"git config color.branch auto"
	"git config user.name '$2'"
	"git config user.email '$3'"
	"git config push.default simple"
	"git config remote.origin.url $4"
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
	#if [ -f $GIT/.git/config.lock ]; then Warning "Présent"; fi
	#rm -f $GIT/.git/config.lock
	eval ${QUERIES[$I]} ||
	    Error "cannot upgrade ($[$I+1]/6) git working directory: $GIT"
    done

    # update bare location
    if [ -n "$4" ]; then
	I=$[I+1]
	Info "${QUERIES[$I]}"
	#if [ -f $GIT/.git/config.lock ]; then Warning "Présent"; fi
	#rm -f $GIT/.git/config.lock
	eval ${QUERIES[$I]} ||
	    Error "cannot upgrade ($[$I+1]/6) git working directory: $GIT" 
    fi
    
    cat >.git/description <<EOF
mediatex's $1 module
EOF
    
    cd - > /dev/null 2>&1 || true
}
    
# this function locally commit change into a module
# $1: git module to commit (MDTX or MDTX-COLL)
# $2: message for this commit
function GIT_commit()
{
    Debug "$FUNCNAME: $1 $2" 2
    [ $# -eq 2 ] || Error "expect 2 parameters"
    GIT=$GITCLT/$1

    local QUERIES=(\
	"git add *[0-9][0-9].txt icons/*.*"
	"[ -z \"\$(git branch)\" ]"
	"git update-index --refresh"
	"git diff-index --exit-code HEAD 2>&1"
	"git commit -a -m \"$2\" 2>&1"
    )
    
    # su to module user if (should not) call from init.sh
    if [ $(id -u) -eq 0 ]; then
	for I in $(seq 0 4); do
	    QUERIES[$I]="su $1 -c '${QUERIES[$I]}'"
	done
    else
	[ "$(whoami)" == $1 ] ||
	    Error "module $1 has to be commited by $1 user"	
    fi
    
    # /var/cache/mediatex/mdtx/git/mdtx-coll
    cd $GIT || Error "cannot cd to git working directory: $GITCLT/$1"

    # add meta-data parts to collections modules
    if [ "$1" != "$MDTX" ]; then
	Info "${QUERIES[0]}"
	eval ${QUERIES[0]} ||
	    Error "cannot commit (1/2) git module: $GITCLT/$1"
    fi

    # check if there is something to commit
    DO_COMMIT=0
    
    # - is it a new module (still no branch) ?
    Info "${QUERIES[1]}"
    if eval ${QUERIES[1]}; then
	DO_COMMIT=1
    else
	# - is there some changes ?
	Info "${QUERIES[2]}"
	eval ${QUERIES[2]} || /bin/true
	Info "${QUERIES[3]}"
	if ! eval ${QUERIES[3]}; then
	    DO_COMMIT=1
	fi
    fi
    
    if [ $DO_COMMIT -eq 0 ]; then
       	Info "nothing to commit into git module $1"
    else
	Info "${QUERIES[4]}"
	eval ${QUERIES[4]} ||
	    Error "cannot commit (2/2) git module: $GITCLT/$1"
    fi
    
    cd - > /dev/null 2>&1 || true
}

# this function update a module
# $1: git module to update (MDTX or MDTX-COLL)
function GIT_pull()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    GIT=$GITCLT/$1
    local QUERY="git pull --no-edit 2>&1"

    # su to module user if (should not) call from init.sh
    if [ $(id -u) -eq 0 ]; then
	QUERY="su $1 -c \"$QUERY\""
    else
	[ "$(whoami)" == $1 ] ||
	    Error "module $1 has to be updated by $1 user"	
    fi
    
    # /var/cache/mediatex/mdtx/git/$1
    cd $GIT || Error "cannot cd to git working directory: $GIT"
	
    Info "$QUERY"
    eval $QUERY ||
	Error "cannot update git working directory: $GIT"    
    
    cd - > /dev/null 2>&1 || true
}

# this function remotely commit change into a module
# $1: git module to commit (MDTX or MDTX-COLL)
function GIT_push()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 2 parameters"

    GIT=$GITCLT/$1
    local QUERY="git push 2>&1"

    # /var/cache/mediatex/mdtx/git/mdtx-coll
    cd $GIT || Error "cannot cd to git working directory: $GITCLT/$1"

    # su to module user when call from init.sh
    if [ $(id -u) -eq 0 ]; then
	QUERY="su $1 -c '$QUERY'"
    else
	[ "$(whoami)" == $1 ] ||
	    Error "module $1 has to be push by $1 user"	
    fi
   
    Info "$QUERY"
    eval $QUERY || Error "cannot push git module: $GITCLT/$1"

    cd - > /dev/null 2>&1 || true
}

# this function initialise a GIT bare repository
# $1: git module to initialise
function GIT_init_gitbare()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    local QUERY="git init --bare $GITBARE/$1 2>&1"
    
    # /var/lib/mediatex/mdtx/$1/HEAD
    if [ -f $GITBARE/$1/HEAD ]; then
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
    GIT=$GITCLT/$MDTX
    local QUERY1="git clone $GITBARE/$MDTX $GITCLT/$MDTX 2>&1"
    local QUERY2="git add -A"

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
	chown -R $1.$1 $GITBARE/$MDTX
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
	cd - > /dev/null 2>&1 || true

	if [ $UNIT_TEST_RUNNING -eq 1 ]; then
            SIGN1="HOSTNAME"
	    SIGN2="FINGERPRINT"
	else
	    SIGN1=$(hostname)
	    SIGN2=$(ssh-keygen -lf /etc/ssh/ssh_host_rsa_key.pub | \
			   cut -d' ' -f2 | sed -e 's/://g')
	fi
	GIT_upgrade $MDTX "$SIGN1" $SIGN2
	GIT_commit $MDTX "Initial mdtx setup"
	GIT_push $MDTX
    fi
}

# this function setup a new collection module
# $1: collection user (MDTX-COLL)
function GIT_coll_import()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    GIT=$GITCLT/$1
    local QUERY1="git clone $GITBARE/$1 $GITCLT/$1 2>&1"
    local QUERY2="git add -A"

    USERS_install $GITBARE/$1 "${_VAR_LIB_M_MDTX_COLL[@]}"
    GIT_init_gitbare $1
    
    # /var/cache/mediatex/mdtx/git/coll/logo
    if [ -f $GIT/logo ]; then
	Warning "re-use already checkouted $1 module"
    else
	Info "su $1 -c \"$QUERY1\""
	cd $GIT || Error "cannot cd to git working directory: $GIT"
	su $1 -c "$QUERY1" || Error "cannot checkout $1 module"
	cd - > /dev/null 2>&1 || true
    fi
    
    if [ -f $GIT/logo ]; then
	Warning "re-use already imported $1 module"
	chown -R $1.$1 $GITBARE/$1
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

	for t in home index score cgi; do
	    install -o $1 -g $1 -m 660 $MISC/$t.htaccess apache2/

	    # adapt them
	    sed apache2/$t.htaccess -i \
		-e "s!MEDIATEX/!$MEDIATEX/!" \
		-e "s!MDTX-COLL!$1!" \
		-e "s!/MDTX/!/$MDTX/!" \
		-e "s!HOMES!$HOMES!"
	done

	cat > .gitignore <<EOF
*NNN.txt
EOF
	chown $1:$1 .gitignore

	# create mdtx password (bypass by unit tests)
	[ $UNIT_TEST_RUNNING -eq 1 ] ||
	    htdigest -c apache2/htpasswd $1 $MDTX

	# import them
       	Info "su $1 -c \"$QUERY2\""
	su $1 -c "$QUERY2" || \
	    Error "cannot add files to $1 module"
	
	cd - > /dev/null 2>&1 || true

	if [ $UNIT_TEST_RUNNING -eq 1 ]; then
            SIGN1="HOSTNAME"
	    SIGN2="FINGERPRINT"
	else
	    SIGN1=$(grep 'host  ' $GITCLT/$MDTX/${MDTX}${CONF_CONFFILE} \
			   | awk '{print $2}')
	    SIGN2=$(ssh-keygen -lf $HOMES/$1$CONF_SSHDIR/id_dsa.pub | \
			   cut -d' ' -f2 | sed -e 's/://g')
	fi
	GIT_upgrade $1 "$SIGN1" $SIGN2
	GIT_commit $1 "Initial collection setup"
	GIT_push $1
    fi
}

# checkout a (potentially remote) collection module
# $1: user
# $2: remote mdtx user
# $3: collection label
# $4: host
function GIT_coll_checkout()
{
    Debug "$FUNCNAME: $1 $2-$3@$4" 2
    [ $# -eq 4 ] || Error "expect 4 parameter"

    GIT=$GITCLT/$1
    MODULE="$2-$3"
    QUERY="git clone ssh://$MODULE@$4:/var/lib/gitbare/$MODULE $GIT 2>&1"
       
    # force checkout using ssh
    rm -fr $GITCLT/$MODULE $GIT
    USERS_install $GIT "${_VAR_LIB_M_MDTX_COLL[@]}"
    
    Info "su $1 -c \"$QUERY\""
    cd $GIT || Error "cannot cd to git working directory: $GIT"
    su $1 -c "$QUERY" || Error "cannot clone $MODULE module"
    cd - > /dev/null 2>&1 || true
}
