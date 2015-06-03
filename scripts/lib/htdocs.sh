#!/bin/bash
#=======================================================================
# * Version: $Id: htdocs.sh,v 1.3 2015/06/03 14:03:26 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the apache and viewvc configuration
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
MDTX_SH_HTDOCS=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# Configure apache2 for mdtx 
function HTDOCS_configure_mdtx_apache2()
{
    Debug "$FUNCNAME:" 2

    # adapt apache2 configuration 
    HTCONF=$SYSCONFDIR/apache2/conf-available$MEDIATEX-$MDTX.conf
    cp $DATADIR/examples/apache-mdtx.conf $HTCONF
    sed $HTCONF -i -e "s!MEDIATEX!$MEDIATEX!"    
    sed $HTCONF -i -e "s!CACHEDIR!$CACHEDIR!"
    sed $HTCONF -i -e "s!MDTX!$MDTX!g"

    # html redirection
    cat > $HOME/public_html/index.html <<EOF
<html>
<head>
<meta http-equiv="refresh" content="0; URL=viewvc.cgi">
</head>
<body>
</html>
EOF
}

# un-Configure apache2 for mdtx 
function HTDOCS_unconfigure_mdtx_apache2()
{
    Debug "$FUNCNAME:" 2

    # adapt apache2 configuration 
    HTCONF=$SYSCONFDIR/apache2/conf-available$MEDIATEX-$MDTX.conf
    rm -f $HTCONF
}

# Configure viewvc for mdtx 
function HTDOCS_configure_mdtx_viewvc()
{
    Debug "$FUNCNAME:" 2

    HOME=$CACHEDIR/$MDTX
    install -o $MDTX -g www-data -m 750 /usr/lib/viewvc/cgi-bin/viewvc.cgi \
	$HOME/public_html
    sed $HOME/public_html/viewvc.cgi -i -e "s!/etc/viewvc/!$HOME/!"

    # viewvc conf
    install -o $MDTX -g www-data -m 640 $DATADIR/examples/viewvc.conf \
	$HOME
    sed $HOME/viewvc.conf -i -e "s!STATEDIR!$STATEDIR!"
    sed $HOME/viewvc.conf -i -e "s!CACHEDIR/MDTX/home/USER!/etc!"
    sed $HOME/viewvc.conf -i -e "s!MDTX!$MDTX!"
    sed $HOME/viewvc.conf -i -e "/forbidden =/ d"
}

# Configure apache2 for a collection 
# $1: user
function HTDOCS_configure_coll_apache2()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    HOME=$CACHEDIR/$MDTX/home/$1
    SSHDIR=$HOME/.ssh
    CVS=$CACHEDIR/$MDTX/cvs/$1
    HTML=$HOME/public_html

    rm -f $HTML/cache
    ln -sf $CACHEDIR/$MDTX/cache/$1 $HTML/cache

    # logo
    ln -sf $CVS/logo.png $HTML/
    ln -sf $CVS/mediatex.css $HTML/
    ln -sf $CVS/icons $HTML/

    # html repositories and links
    for t in cgi index cache score; do
        if [ $t != cache ]; then 
	    install -o $MDTX -g $1 -m 750 -d $HTML/$t
	fi
	ln -sf $CVS/apache2/$t.htaccess $HTML/$t/.htaccess
    done
    ln -sf $CVS/apache2/home.htaccess $HTML/.htaccess

    # /etc links in order to access .htpasswd from .htaccess
    rm -f $ETCDIR/$1
    ln -sf $CVS $ETCDIR/$1

    # cgi scripts
    ln -sf $DATADIR/cgi-dir/cgi $HTML/cgi/get.cgi

    # html redirection
    cat > $HTML/index.shtml <<EOF
<html>
<head>
<meta http-equiv="refresh" content="0; URL=index">
</head>
<body>
</html>
EOF
   
    chown $MDTX:$1 $HTML/index.shtml
}

# Configure viewvc for a collection
# $1: user
function HTDOCS_configure_coll_viewvc()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    EXAMPLE=$DATADIR/examples

    # viewvc
    HOME=$CACHEDIR/$MDTX/home/$USER
    install -o $USER -g www-data -m 750 \
	/usr/lib/viewvc/cgi-bin/viewvc.cgi \
	$HOME/public_html/cgi/viewvc.cgi
    sed $HOME/public_html/cgi/viewvc.cgi -i -e "s!/etc/!$HOME/!"
    
    # viewvc template and conf
    install -m 750 -o $USER -g www-data -d $HOME/viewvc
    install -o $USER -g www-data -m 640 $EXAMPLE/viewvc.conf \
	$HOME/viewvc
    sed $HOME/viewvc/viewvc.conf -i -e "s!STATEDIR!$STATEDIR!"
    sed $HOME/viewvc/viewvc.conf -i -e "s!CACHEDIR!$CACHEDIR!"
    sed $HOME/viewvc/viewvc.conf -i -e "s!MDTX!$MDTX!"
    sed $HOME/viewvc/viewvc.conf -i -e "s!USER!$USER!"

    # modify localy header.ezt
    cp -fr /etc/viewvc/templates $HOME/viewvc
    install -m 640 -o $USER -g www-data \
	$EXAMPLE/header.ezt $HOME/viewvc/templates/include
    sed $HOME/viewvc/templates/include/header.ezt -i -e \
	"s!PATH!/~$USER!"
}

# Unitary test :
# - check data remote copy
# - check metadata sharing using a jail
if UNIT_TEST_start "htdocs"; then
    [ ! -z $MDTX_SH_USERS ] || source $libdir/users.sh   
 
    MDTX="ut5-mdtx"
    COLL="hello"
    USER="$MDTX-$COLL"
    
    # cleanup if previous test has failed
    USERS_mdtx_remove_user
    USERS_coll_remove_user $USER

    # init.sh
    USERS_root_populate
    USERS_mdtx_create_user
    HTDOCS_configure_mdtx_apache2
    HTDOCS_configure_mdtx_viewvc

    # new.sh
    USERS_coll_create_user $USER
    HTDOCS_configure_coll_apache2 $USER
    HTDOCS_configure_coll_viewvc $USER

    # free.sh/purge.sh
    USERS_coll_remove_user $USER
    USERS_mdtx_remove_user

    Info "success"
    UNIT_TEST_stop "htdocs"
fi
