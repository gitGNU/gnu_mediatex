#!/bin/bash
#=======================================================================
# * Version: $Id: htdocs.sh,v 1.4 2015/06/30 17:37:23 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * This module manage the apache and viewvc configuration
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
MDTX_SH_HTDOCS=1
[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/scripts/lib
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh

# Configure apache2 for mdtx 
function HTDOCS_configure_mdtx_apache2()
{
    Debug "$FUNCNAME:" 2

    # adapt apache2 configuration 
    HTCONF=$SYSCONFDIR/apache2/conf-available$MEDIATEX-$MDTX.conf
    cp $MISC/apache-mdtx.conf $HTCONF
    sed $HTCONF -i \
	-e "s!MEDIATEX!$MEDIATEX!" \
	-e "s!CACHEDIR!$CACHEDIR!" \
	-e "s!MDTX!$MDTX!g"

    # html redirection
    cat > $MDTXHOME$CONF_HTMLDIR/index.html <<EOF
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

    # remove apache2 configuration 
    HTCONF=$SYSCONFDIR/apache2/conf-available$MEDIATEX-$MDTX.conf
    rm -f $HTCONF
}

# Configure viewvc for mdtx 
function HTDOCS_configure_mdtx_viewvc()
{
    Debug "$FUNCNAME:" 2

    install -o $MDTX -g $MDTX -m 750 \
	/usr/lib/viewvc/cgi-bin/viewvc.cgi $MDTXHOME/public_html
    sed $MDTXHOME/public_html/viewvc.cgi -i \
	-e "s!/etc/viewvc/!$MDTXHOME/!"

    # viewvc conf
    install -o $MDTX -g $MDTX -m 640 $MISC/viewvc.conf $MDTXHOME
    sed $MDTXHOME/viewvc.conf -i \
	-e "s!CVSROOT!$CVSROOT!" \
	-e "s!HOMES/USER!/etc!" \
	-e "s!MDTX!$MDTX!" \
	-e "/forbidden =/ d"
}

# Configure apache2 for a collection 
# $1: collection user
function HTDOCS_configure_coll_apache2()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    COLL_HOME=$HOMES/$1
    COLL_SSH=$COLL_HOME$CONF_SSHDIR
    COLL_CVS=$CVSCLT/$1
    COLL_HTML=$COLL_HOME/public_html

    rm -f $COLL_HTML/cache
    ln -sf $CACHES/$1 $COLL_HTML/cache

    # custumize web site
    ln -sf $COLL_CVS/logo $COLL_HTML/
    ln -sf $COLL_CVS/mediatex.css $COLL_HTML/
    ln -sf $COLL_CVS/readme.html $COLL_HTML/
    ln -sf $COLL_CVS/icons $COLL_HTML/

    # html repositories and links
    for t in cgi index cache score; do
        if [ $t != cache ]; then 	    
	    install -o $MDTX -g $MDTX -m 750 -d $COLL_HTML/$t
	fi
	ln -sf $COLL_CVS/apache2/$t.htaccess $COLL_HTML/$t/.htaccess
    done
    ln -sf $COLL_CVS/apache2/home.htaccess $COLL_HTML/.htaccess

    # /etc links in order to access .htpasswd from .htaccess
    rm -f $ETCDIR/$1
    ln -sf $COLL_CVS $ETCDIR/$1

    # cgi scripts
    ln -sf $DATADIR/cgi-dir/cgi $COLL_HTML/cgi/get.cgi

    # html redirection
    cat > $COLL_HTML/index.shtml <<EOF
<html>
<head>
<meta http-equiv="refresh" content="0; URL=index">
</head>
<body>
</html>
EOF
   
    chown $MDTX:$1 $COLL_HTML/index.shtml
}

# Configure viewvc for a collection
# $1: collection user
function HTDOCS_configure_coll_viewvc()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    # viewvc
    COLL_HOME=$HOMES/$1
    install -o $MDTX -g $MDTX -m 750 \
	/usr/lib/viewvc/cgi-bin/viewvc.cgi \
	$COLL_HOME/public_html/cgi/viewvc.cgi
    sed $COLL_HOME/public_html/cgi/viewvc.cgi -i \
	-e "s!/etc/!$COLL_HOME/!"
    
    # viewvc template and conf
    install -o $MDTX -g $MDTX -m 750 -d $COLL_HOME/viewvc
    install -o $MDTX -g $MDTX -m 640 $MISC/viewvc.conf \
	$COLL_HOME/viewvc
    sed $COLL_HOME/viewvc/viewvc.conf -i \
	-e "s!CVSROOT!$CVSROOT!" \
	-e "s!HOMES!$HOMES!" \
	-e "s!USER!$1!"

    # modify localy header.ezt
    cp -fr /etc/viewvc/templates $COLL_HOME/viewvc
    install -o $MDTX -g $MDTX -m 640 \
	$MISC/header.ezt $COLL_HOME/viewvc/templates/include
    sed $COLL_HOME/viewvc/templates/include/header.ezt -i \
	-e "s!PATH!/~$1!"
}
