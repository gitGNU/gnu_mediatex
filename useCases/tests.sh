#!/bin/bash
#=======================================================================
# * Version: $Id: tests.sh,v 1.2 2014/11/13 16:37:14 nroche Exp $
# * Project: MediaTex
# * Module : post installation tests
# *
# * This script will performe tests detailed into the documentation
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

# $1 topo to display
function topo()
{
    echo "**********************************"
    echo " test $TEST_NB: $1"
    echo "**********************************"
    logger -p local2.NOTICE "**********************************"
    logger -p local2.NOTICE " test $TEST_NB: $1"
    logger -p local2.NOTICE "**********************************"
}

# $1 query to display
# $2 mediatexd user (serv1 by default)
function query()
{
    QUERY=$1
    SERVER=${2-serv1}
    echo "++++++++++++++++++++++++++++++++++"
    echo " $SERVER: $QUERY"
    echo "++++++++++++++++++++++++++++++++++"
    logger -p local2.NOTICE "++++++++++++++++++++++++++++++++++"
    logger -p local2.NOTICE " $SERVER: $QUERY"
    logger -p local2.NOTICE "++++++++++++++++++++++++++++++++++"
}

# $1 admin query
# $2 mediatexd user (serv1 by default)
function mdtxA()
{
    QUERY=$1
    SERVER=${2-serv1}
    query "$QUERY" $SERVER
    mediatex -swarning -c $SERVER $QUERY
}

# $1 publisher query
# $2 mediatexd user (serv1 by default)
function mdtxP()
{
    QUERY=$1
    SERVER=${2-serv1}
    query "$QUERY" $SERVER
    mediatex -swarning -c $SERVER su <<EOF
mediatex $QUERY
EOF
}

# $1 question to ask
# $2 command to exec before
function question()
{
    if [ ! -z "$2" ]; then
	echo "=> $2 <="
	echo "----------------------------------"
	eval $2
	echo "----------------------------------"
    fi
    read -p "> $1 (y)/n: " VALUE
    case "$VALUE" in
	n|N)
	    echo "bye"
	    exit
	    ;;
    esac
}

# $1 note to display
# $2 command to exec before
function notice()
{
    if [ ! -z "$2" ]; then
	echo "=> $2 <="
	echo "----------------------------------"
	eval $2
	echo "----------------------------------"
    fi
    echo "> $1"
    read -p "> push a key to continue"
}

function yourMail()
{
    echo "> browse https://localhost/~serv1-hello"
    notice "ask for the logo file and give an email address."
}

# $1 question to ask
# $2 file to display
function finalQuestion()
{
    echo "**********************************"
    question "$1" "$2"
    TEST_NB=$[TEST_NB+1]
    echo $TEST_NB > /tmp/test.nb   
}

# prenvent sed to replace a symbolic link
# $1 pattern "s/x/y/"
# $2 file to process
function sedInPlace()
{
    file=$(mktemp)
    sed -e "$1" $2 > $file
    cp $file $2
    rm $file
}

############################################
TEST_NB=1
[ -f /tmp/test.nb ] && TEST_NB=$(cat /tmp/test.nb)

# check being root
[ $(id -u) -ne 0 ] && {
    echo "** you need to be root for this test"
    exit
}

# check no hello users are logged
ps -ef | grep h[e]llo
[ $? -ne 1 ] && {
    echo "** Please logout hello users"
    exit
}

############################################
if [ "$TEST_NB" -le 1 ]; then
    topo "Configuring"
    #/etc/init.d/mediatexd stop
    mdtxA "adm remove"
    mdtxA "adm purge"
    mdtxA "adm init"
    sedInPlace "s/6561/6001/" /etc/mediatex/serv1.conf
    finalQuestion "is http://localhost/~serv1/ correct ?"
fi

############################################
if [ $TEST_NB -le 2 ]; then
    topo "Dealing with locals supports"
    mdtxP "ls supp"
    mdtxP "note supp ex-cd1 as cd1"
    mdtxP "add supp iso1 on /usr/share/mediatex/examples/logoP1.iso"
    mdtxP "add supp iso2 on /usr/share/mediatex/examples/logoP2.iso"  
    mdtxP "check supp ex-cd1 on /usr/share/mediatex/examples/logoP1.iso"  
    mdtxP "check supp iso1 on /usr/share/mediatex/examples/logoP1.iso"
    mdtxP "check supp iso2 on /usr/share/mediatex/examples/logoP2.iso"
    mdtxP "ls supp"
    finalQuestion "is support db updated ?" \
	"cat /var/cache/mediatex/serv1/cvs/serv1/supports.txt"
fi

############################################
if [ $TEST_NB -le 3 ]; then
    topo "Hello collection"
    mdtxA "adm add coll hello@localhost"
    mdtxP "make coll hello"
    finalQuestion "is https://localhost/~serv1-hello/ correct ? (login is serv1)"
fi

############################################
if [ $TEST_NB -le 4 ]; then
    topo "Registering mail"
    /etc/init.d/mediatexd status serv1
    yourMail
    mdtxP "srv save"
    finalQuestion "do the server get it ?" \
	"cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
fi

############################################
if [ $TEST_NB -le 5 ]; then
    topo "Providing supports"
    mdtxP "add supp iso1 to coll hello"
    mdtxP "add supp iso2 to coll hello"
    question "does the configuration mention supports ?" \
	"cat /etc/mediatex/serv1.conf"
    mdtxP "motd"
    mdtxP "check supp iso1 on /usr/share/mediatex/examples/logoP1.iso"
    mdtxP "check supp iso2 on /usr/share/mediatex/examples/logoP2.iso"
    mdtxP "srv save"
    finalQuestion "check your mailbox" \
	"cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
fi

############################################
if [ $TEST_NB -le 6 ]; then
    topo "Upload file"
    # setup
    mdtxP "upload /usr/share/mediatex/examples/mediatex.css to coll hello"
    mdtxP "make coll hello"
    finalQuestion "browse score index and download mediatex.css"
fi

############################################
if [ $TEST_NB -le 7 ]; then
    topo "2nd server"
    # clean
    /etc/init.d/mediatexd stop serv2
    # setup
    mdtxA "adm purge" serv2
    mdtxA "adm init" serv2
    sedInPlace "s/6561/6002/" /etc/mediatex/serv2.conf
    sedInPlace "s/www/www, private/" /etc/mediatex/serv2.conf
    echo "Gateways private" >> /etc/mediatex/serv2.conf
    /etc/init.d/mediatexd status serv2
    finalQuestion "is 2nd server running ?" \
	"/etc/init.d/mediatexd status serv2"
fi

############################################
if [ $TEST_NB -le 8 ]; then
    topo "2nd server register to hello collection"
    # clean
    SRV="/var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
    FPS=$(grep -B4 6562 $SRV | grep Server | awk '{ print $2 }')
    for FP in $FPS; do 
	mdtxP "del key $FP from coll hello" serv1
    done
    mdtxA "adm del coll hello" serv2
    # setup
    notice "you will be ask for this fingerprint" \
	"grep 'host fingerprint' /etc/mediatex/serv1.conf"
    mdtxA "adm add coll serv1-hello" serv2
    # need to be accepted first
    cp /var/cache/mediatex/serv2/home/serv2-hello/.ssh/id_dsa.pub /tmp/key
    mdtxP "add key /tmp/key to coll hello" serv1
    # now connect
    mdtxA "adm add coll serv1-hello" serv2
    question "connected to serv1 cvsroot ?" \
	"cat /var/cache/mediatex/serv2/cvs/serv2-hello/CVS/Root"
    # server must be aware of belonging to serv2-hello group
    /etc/init.d/mediatexd restart serv2
    # seen by first server
    mdtxP "upgrade" serv1
    finalQuestion "does 1st server see 2nd server ?" \
	"grep Server /var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
fi

############################################
if [ $TEST_NB -le 9 ]; then
    topo "serv1 notify to 2nd server"
    # clean
    rm -fr /var/cache/mediatex/serv1/cache/serv1-hello/logo*
    if /etc/init.d/mediatexd status serv1; then
	/etc/init.d/mediatexd reload serv1
    else
	/etc/init.d/mediatexd status serv1
    fi
    yourMail
    mdtxP "srv save" serv1
    question "do the 1st server get it ?" \
	"cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
    mdtxP "srv notify" serv1
    mdtxP "srv save" serv2
    finalQuestion "does 2nd server get your demand ?" \
	"cat /var/cache/mediatex/serv2/md5sums/serv2-hello.md5"
fi

############################################
if [ $TEST_NB -le 10 ]; then
    topo "2nd have a support and notify to serv1"
    mdtxP "add supp iso1 on /usr/share/mediatex/examples/logoP1.iso" \
	serv2
    mdtxP "add supp iso1 to coll hello" serv2
    mdtxP "check supp iso1 on /usr/share/mediatex/examples/logoP1.iso" \
	serv2
    mdtxP "srv notify" serv2
    
    #mdtxP "srv save" serv1
    #question "do the 2nd server notify it got it ?" \
	#"cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"

    # serv1 scp the remote content
    mdtxP "srv extract" serv1
    mdtxP "srv save" serv1
    finalQuestion "part1 remotely copied by serv1 ?" \
	"cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
fi

############################################
if [ $TEST_NB -le 11 ]; then
    topo "3rd (Nat client) server"
    # clean
    /etc/init.d/mediatexd stop serv3
    # setup
    mdtxA "adm purge" serv3
    mdtxA "adm init" serv3
    sedInPlace "s/6561/6003/" /etc/mediatex/serv3.conf
    sedInPlace "s/www/private/" /etc/mediatex/serv3.conf
    /etc/init.d/mediatexd status serv3
    finalQuestion "is 3rd server running ?" \
	"/etc/init.d/mediatexd status serv3"
fi

############################################
if [ $TEST_NB -le 12 ]; then
    topo "3rd server register to hello collection"
    # clean
    SRV="/var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
    FPS=$(grep -B4 6563 $SRV | grep Server | awk '{ print $2 }')
    for FP in $FPS; do 
	mdtxP "del key $FP from coll hello" serv1
    done
    mdtxA "adm del coll hello" serv3
    # setup
    notice "you will be ask for this fingerprint" \
	"grep 'host fingerprint' /etc/mediatex/serv1.conf"
    mdtxA "adm add coll serv1-hello" serv3
    # need to be accepted first
    cp /var/cache/mediatex/serv3/home/serv3-hello/.ssh/id_dsa.pub /tmp/key
    mdtxP "add key /tmp/key to coll hello" serv1
    # now connect
    mdtxA "adm add coll serv1-hello" serv3
    question "connected to serv1 cvsroot ?" \
	"cat /var/cache/mediatex/serv3/cvs/serv3-hello/CVS/Root"
    # server must be aware of belonging to serv3-hello group
    /etc/init.d/mediatexd restart serv3
    # seen by first server
    mdtxP "upgrade" serv1
    finalQuestion "does 1st server see 3rd server ?" \
	"grep Server /var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
fi

############################################
# if [ $TEST_NB -le 13 ]; then
#     topo "set-up serv3 client/server link"
#     mdtxP "upgrade coll hello" serv2
#     SRV_SERV2="/var/cache/mediatex/serv2/cvs/serv2-hello/servers.txt"
#     SRV_SERV3="/var/cache/mediatex/serv3/cvs/serv3-hello/servers.txt"
#     FP_SERV2=$(grep -B4 6002 $SRV_SERV3 | grep Server | awk '{ print $2 }')
#     FP_SERV3=$(grep -B4 6003 $SRV_SERV2 | grep Server | awk '{ print $2 }')
#     # setup
#     mdtxP "add natSrv $FP_SERV2 to coll hello" serv3
#     mdtxP "add natClt $FP_SERV3 to coll hello" serv2
#     finalQuestion "are servers linked ?" "grep nat $SRV_SERV2"
# fi

############################################
if [ $TEST_NB -le 14 ]; then
    topo "serv1 notify to 3rd server"
    # clean
    mdtxP "upgrade" serv3
    mdtxP "upgrade" serv2
    mdtxP "upgrade" serv1
    sleep 1
    mdtxP "srv notify" serv1
    question "please check the logs" "tail -n 40 /var/log/mediatex.log | grep -i notify"
    mdtxP "srv save" serv3
    finalQuestion "does 3rd server get your demand ?" \
	"cat /var/cache/mediatex/serv3/md5sums/serv3-hello.md5"
fi

############################################
if [ $TEST_NB -le 15 ]; then
    topo "3rd have a support and notify to serv1"
    mdtxP "add supp iso2 on /usr/share/mediatex/examples/logoP2.iso" \
	serv3
    mdtxP "add supp iso2 to coll hello" serv3
    mdtxP "check supp iso2 on /usr/share/mediatex/examples/logoP2.iso" \
	serv3
    mdtxP "srv save" serv3
    question "does serv3 get part2 ?" \
	"cat /var/cache/mediatex/serv3/md5sums/serv3-hello.md5"
    mdtxP "srv notify" serv3
    mdtxP "srv extract" serv2
    mdtxP "srv save" serv2
    finalQuestion "part2 remotely copied by serv2 ?" \
	"cat /var/cache/mediatex/serv2/md5sums/serv2-hello.md5"
fi

############################################
if [ $TEST_NB -le 16 ]; then
    topo "move repository from serv1 to serv2"
    if [ -d /var/lib/mediatex/serv1/serv1-hello ]; then
	rm -fr /var/lib/mediatex/serv2/serv2-hello
	mv /var/lib/mediatex/serv1/serv1-hello \
	    /var/lib/mediatex/serv2/serv2-hello
    fi
    notice "you will be ask for localhost fingerprint..."
    mdtxA "adm add coll hello" serv2
    question "connected to serv2 cvsroot ?" \
	"cat /var/cache/mediatex/serv2/cvs/serv2-hello/CVS/Root"
    mdtxA "adm add coll serv2-hello" serv1
    question "connected to serv2 cvsroot ?" \
	"cat /var/cache/mediatex/serv1/cvs/serv1-hello/CVS/Root"
    mdtxA "adm add coll serv2-hello" serv3
    finalQuestion "connected to serv2 cvsroot ?" \
	"cat /var/cache/mediatex/serv3/cvs/serv3-hello/CVS/Root"
fi

############################################
echo "-> tests success"
echo "(cleaning...)"
mdtxA "adm purge" serv1
mdtxA "adm purge" serv2
mdtxA "adm purge" serv3
rm -f /tmp/test.nb










