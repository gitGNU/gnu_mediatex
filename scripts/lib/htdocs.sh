#!/bin/bash
#=======================================================================
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
    HTCONF=$SYSCONFDIR/apache2/conf-available$MEDIATEX-$MDTX.conf
    MDTX_HTML=$MDTXHOME$CONF_HTMLDIR
    
    # adapt apache2 configuration 
    cp $MISC/apache-mdtx.conf $HTCONF
    sed $HTCONF -i \
	-e "s!MEDIATEX!$MEDIATEX!" \
	-e "s!CACHEDIR!$CACHEDIR!" \
	-e "s!MDTX!$MDTX!g"

    # html redirection
    cat > $MDTX_HTML/index.html <<EOF
<html>
<head>
<meta http-equiv="refresh" content="0; URL=cgit.cgi">
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

# Configure cgit for mdtx 
function HTDOCS_configure_mdtx_cgit()
{
    Debug "$FUNCNAME:" 2

    ln -sf /usr/lib/cgit/cgit.cgi $MDTXHOME/public_html
    ln -sf /usr/share/mediatex/misc/logo $MDTXHOME/public_html
    cat >$MDTXHOME/cgitrc <<EOF
css=/cgit-css/cgit.css
logo=/~$MDTX/logo
scan-path=$GITCLT
EOF
}

# Configure apache2 for a collection 
# $1: collection user
function HTDOCS_configure_coll_apache2()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"

    COLL_HOME=$HOMES/$1
    COLL_SSH=$COLL_HOME$CONF_SSHDIR
    COLL_GIT=$GITCLT/$1
    COLL_HTML=$COLL_HOME/$CONF_HTMLDIR

    rm -f $COLL_HTML/cache
    ln -sf $CACHES/$1 $COLL_HTML/cache

    # custumize web site
    ln -sf $COLL_GIT/logo $COLL_HTML/
    ln -sf $COLL_GIT/mediatex.css $COLL_HTML/
    ln -sf $COLL_GIT/readme.html $COLL_HTML/
    ln -sf $COLL_GIT/icons $COLL_HTML/

    # html repositories and links
    for t in cgi index score; do
	install -o $MDTX -g $MDTX -m 750 -d $COLL_HTML/$t
	ln -sf $COLL_GIT/apache2/$t.htaccess $COLL_HTML/$t/.htaccess
    done
    ln -sf $COLL_GIT/apache2/home.htaccess $COLL_HTML/.htaccess

    # /etc links in order to access htpasswd from .htaccess
    rm -f $ETCDIR/$1
    ln -sf $COLL_GIT $ETCDIR/$1

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

# Configure cgit for collection
# $1: collection user
function HTDOCS_configure_coll_cgit()
{
    Debug "$FUNCNAME: $1" 2
    [ $# -eq 1 ] || Error "expect 1 parameter"
    COLL_HOME=$HOMES/$1
    
    ln -sf /usr/lib/cgit/cgit.cgi $COLL_HOME/public_html/cgi
    ln -sf ..${CONF_GITCLT}/logo $COLL_HOME/public_html
    cat >$COLL_HOME/cgitrc <<EOF
css=/cgit-css/cgit.css
logo=/~$1/logo
scan-path=$GITCLT/$1
header=${COLL_HOME}${CONF_HTMLDIR}/gitHeader.html
noheader=1
EOF
}
