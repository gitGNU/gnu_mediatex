#!/bin/bash
#=======================================================================
# * Version: $Id: tests.sh,v 1.9 2015/08/30 17:08:00 nroche Exp $
# * Project: MediaTex
# * Module : post installation tests
# *
# * This script will performe tests detailed into the documentation,
# * but by using multiple server instances instead of hosted ones.
# *
# * Note : this script accept the "clean" argument to restart tests
# *        from the beginning
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

############################################
# remanent tasks
############################################

## demo
SEVERITY_CLIENT="-s notice"
#SEVERITY_SERVER="-s notice"
DEBUG_CLIENT_SCRIPT=""
DEBUG_SERVER=0

## debug
#SEVERITY_CLIENT="-s debug"
SEVERITY_SERVER="-s notice -s info:main"
#DEBUG_CLIENT_SCRIPT="-S"
#DEBUG_SERVER=1          # need to run "$ xhost +" first
#ADDON_SERVER=valgrind

### not done
## ADDON_SERVER=gdb
## cf: gdb -ex "set args -c serv1" -ex "r" mediatexd

# $1 topo to display
function topo()
{
    echo "*************************************************************"
    echo " test $TEST_NB: $1"
    echo "*************************************************************"
    logger -p local2.NOTICE "******************************************"
    logger -p local2.NOTICE " test $TEST_NB: $1"
    logger -p local2.NOTICE "******************************************"
}

# $1 query to display
# $2 mdtx user (serv1 by default)
function query()
{
    QUERY=$1
    SERVER=${2-serv1}
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo " $SERVER: $QUERY"
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    logger -p local2.NOTICE "++++++++++++++++++++++++++++++++++++++++++"
    logger -p local2.NOTICE " $SERVER: $QUERY"
    logger -p local2.NOTICE "++++++++++++++++++++++++++++++++++++++++++"
}

# $1 admin query
# $2 mdtx user (serv1 by default)
function mdtxA()
{
    QUERY=$1
    SERVER=${2-serv1}
    query "$QUERY" $SERVER
    mediatex -c $SERVER $SEVERITY_CLIENT $DEBUG_CLIENT_SCRIPT $QUERY 
}

# $1 publisher query
# $2 mdtx user (serv1 by default)
function mdtxP()
{
    QUERY=$1
    SERVER=${2-serv1}
    query "$QUERY" $SERVER
    mediatex -c $SERVER $SEVERITY_CLIENT $DEBUG_CLIENT_SCRIPT su <<EOF
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
	    echo "Sorry, please fix me and retry..."
	    TEST_OK=0
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
}

function yourMail()
{
    echo "> Please, browse https://localhost/~serv1-hello"
    notice "ask for the logo file and give an email address."
    read -p "> push a key to continue"
}

# $1 question to ask
# $2 file to display
function finalQuestion()
{
    echo "**********************************"
    question "$1" "$2"
    if [ $TEST_OK -eq 1 ]; then
	TEST_NB=$[TEST_NB+1]
    fi
}

# prevent sed to replace a symbolic link
# $1 pattern "s/x/y/"
# $2 file to process
function sedInPlace()
{
    file=$(mktemp)
    sed -e "$1" $2 > $file
    cp $file $2
    rm $file
}

# add init script for new server 
# (as sytemd cannot handle a single one)
# $1 mdtx user (serv1 by default)
function startInitdScript()
{
    SERVER=${1-serv1}

    query "start" $SERVER
    
    # replace localhost by 127.0.0.1 in configuration
    sedInPlace "s/\(host.*\)localhost/\1127.0.0.1/" \
	/etc/mediatex/$SERVER.conf

    # copy initd script
    sed -e "s|mediatexd|mediatexd-$SERVER|" \
	-e "s|\(DAEMON=/usr/bin\)/.*|\1/mediatexd|" \
	-e "s|mdtx|$SERVER|" \
	-e "s|--severity notice|$SEVERITY_SERVER|" \
	/etc/init.d/mediatexd > /etc/init.d/mediatexd-$SERVER
    chmod +x /etc/init.d/mediatexd-$SERVER
    systemctl daemon-reload # needed by systemd

    if [ $DEBUG_SERVER -eq 0 ]; then
	/etc/init.d/mediatexd-$SERVER start
    else 
	cat >/tmp/doNotClose.sh <<EOF
#!/bin/bash
echo "---- run: \$@ ----"
\$@
echo "---- exited ----"
bash
EOF
	chmod +x /tmp/doNotClose.sh
	su $SERVER -c \
	    "env -u SESSION_MANAGER xterm -e \
              /tmp/doNotClose.sh $ADDON_SERVER mediatexd \
               -c $SERVER $SEVERITY_SERVER -ffile -S &"	    
	sleep 1
	mkdir -p /var/run/mediatex

	case "$ADDON_SERVER" in
	    valgrind)
		PS=$(ps -ef | grep "valgrind.bin [m]ediatexd -c $SERVER")
		;;
	    *)
		PS=$(ps -ef | grep "0 [m]ediatexd -c $SERVER")
	esac

	echo $PS | awk '{print $2}' >/var/run/mediatex/${SERVER}d.pid
    fi
}

# $1 mdtx user (serv1 by default)
function statusInitdScript()
{
    SERVER=${1-serv1}

    query "status" $SERVER

    if [ $DEBUG_SERVER -eq 0 ]; then
	/etc/init.d/mediatexd-$SERVER status
    else
	ps -ef | grep $(cat /var/run/mediatex/${SERVER}d.pid)
    fi
}


# $1 mdtx user (serv1 by default)
function stopInitdScript()
{
    SERVER=${1-serv1}

    query "stop" $SERVER

    if [ $DEBUG_SERVER -eq 0 ]; then
	if [ -f /etc/init.d/mediatexd-$SERVER ]; then
	    /etc/init.d/mediatexd-$SERVER stop
	fi
    else
	if [ -f /var/run/mediatex/${SERVER}d.pid ]; then
	    kill -TERM $(cat /var/run/mediatex/${SERVER}d.pid)
	    rm /var/run/mediatex/${SERVER}d.pid
	fi
    fi

    rm -f /etc/init.d/mediatexd-$SERVER
}

# $1 mdtx user (serv1 by default)
function reloadInitdScript()
{
    SERVER=${1-serv1}

    query "reload" $SERVER

    if [ $DEBUG_SERVER -eq 0 ]; then
	if [ -f /etc/init.d/mediatexd-$SERVER ]; then
	    /etc/init.d/mediatexd-$SERVER reload
	fi
    else
	if [ -f /var/run/mediatex/${SERVER}d.pid ]; then
	    kill -HUP $(cat /var/run/mediatex/${SERVER}d.pid)
	fi
    fi
}

# $1 mdtx user (serv1 by default)
function restartInitdScript()
{
    SERVER=${1-serv1}

    query "restart" $SERVER

    if [ $DEBUG_SERVER -eq 0 ]; then
	if [ -f /etc/init.d/mediatexd-$SERVER ]; then
	    /etc/init.d/mediatexd-$SERVER restart
	fi
    else
	stopInitdScript $SERVER
	if [ x"$ADDON_SERVER" == "valgrind" ]; then
	    sleep 3
	fi
	startInitdScript $SERVER
    fi
}

############################################
# tests
############################################

function cleanAll()
{
    topo "Clean all"
    for SERV in serv1 serv2 serv3; do
	stopInitdScript $SERV
	mdtxA "adm purge" $SERV
    done
    rm -f /tmp/test.nb
}
    
# Configure and initialise server 1
function test1()
{
    if [ "x$1" != "xclean" ]; then
	topo "Configuring"
	mdtxA "adm init"
	sedInPlace "s/6561/6001/" /etc/mediatex/serv1.conf
	startInitdScript
	finalQuestion "is http://localhost/~serv1/ correct ?"
    else
	topo "Cleanup"
	stopInitdScript
	mdtxA "adm remove"
	mdtxA "adm purge"
    fi
}

# Add some supports
function test2()
{
    if [ "x$1" != "xclean" ]; then
	topo "Dealing with locals supports"
	mdtxP "ls supp"
	mdtxP "note supp ex-cd1 as cd1"
	mdtxP "add supp iso1 on /usr/share/mediatex/misc/logoP1.iso"
	mdtxP "add file /usr/share/mediatex/misc/logoP2.iso"  
	mdtxP "check supp ex-cd1 on /usr/share/mediatex/misc/logoP1.iso"  
	mdtxP "check supp iso1 on /usr/share/mediatex/misc/logoP1.iso"
	mdtxP "ls supp"
	finalQuestion "is support db updated ?" \
		      "cat /var/cache/mediatex/serv1/cvs/serv1/supports.txt"
    else
	topo "Cleanup"
	mdtxP "del supp iso2"
	mdtxP "del supp /usr/share/mediatex/misc/logoP2.iso"
	mdtxP "note supp ex-cd1 as ok"
    fi
}

# Create "hello" collection on server1
function test3()
{
    if [ "x$1" != "xclean" ]; then
	topo "Create 'Hello' collection on server1"
	mdtxA "adm add coll hello@localhost"
	
	# workaround for multiple server instances (because of systemd)
	restartInitdScript
	
	mdtxP "make coll hello"
	finalQuestion \
	    "is https://127.0.0.1/~serv1-hello/ correct ? (login is serv1)"
    else
	topo "Cleanup"
	mdtxA "adm del coll hello"

	# here we may want to remove the CVS content too (or not)
	# rm -fr /var/lib/mediatex/serv1/serv1-hello
    fi
}

# Ask for an archive record not yet available and register your mail
function test4()
{
    if [ "x$1" != "xclean" ]; then
	topo "Ask for an archive record not yet available and register your mail"
	echo "123456789012345" | netcat 127.0.0.1 6001
	statusInitdScript
	question "does server survive after a strange message ?"
	[ $TEST_OK -eq 0 ] && return

	yourMail
	mdtxP "srv save"
	finalQuestion "does the server get it ?" \
		      "cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
    else
	topo "Cleanup"
	stopInitdScript
	rm -f /var/cache/mediatex/serv1/md5sums/serv1-hello.md5
	startInitdScript
    fi
}

# Provide supports to get the requested archive record
function test5()
{
    if [ "x$1" != "xclean" ]; then
    topo "Provide supports to get the requested archive record"
    mdtxP "add supp iso1 to coll hello"
    mdtxP "add supp /usr/share/mediatex/misc/logoP2.iso to coll hello"
    question "does the configuration mention supports ?" \
	     "grep -A10 Collection /etc/mediatex/serv1.conf"
    [ $TEST_OK -eq 0 ] && return
    
    mdtxP "motd"
    mdtxP "check supp iso1 on /usr/share/mediatex/misc/logoP1.iso"
    mdtxP "srv save"
    question "check your mailbox" \
		  "cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"*
    [ $TEST_OK -eq 0 ] && return

    mdtxP "srv extract"
    mdtxP "srv save"
    finalQuestion "have simply extract from support. Extract do more ?" \
	"cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
    else
	topo "Cleanup"
	mdtxP "del supp iso1 from coll hello"
	mdtxP "del supp /usr/share/mediatex/misc/logoP2.iso from coll hello"
	rm -fr /var/cache/mediatex/serv1/cache/serv1-hello/logo*
	rm -fr /var/cache/mediatex/serv1/cache/serv1-hello/supports
	reloadInitdScript
    fi
}

# Upload a file
function test6()
{
    if [ "x$1" != "xclean" ]; then
	FILE=/usr/share/mediatex/misc/mediatex.css
	SIGN=$(md5sum $FILE | cut -d' ' -f 1)
	SIGN=$SIGN:$(ls $FILE -l | cut -d' ' -f 5)
	cat >/tmp/test.cat <<EOF
Category "css": "drawing"

Document "css": "css"
  With "designer" = "Me" ""
  $SIGN
EOF
	topo "Upload file"
	mdtxP "upload file $FILE catalog /tmp/test.cat to coll hello"
	mdtxP "make coll hello"
	finalQuestion "Please, browse score index and download mediatex.css"
	[ $TEST_OK -eq 0 ] && return
	
	# clean extracted files (~test5) on cache for next tests
	topo "remove support iso2 and extract files"
	mdtxP "del supp /usr/share/mediatex/misc/logoP2.iso from coll hello"
	mdtxP "upgrade+"
	rm -fr /var/cache/mediatex/serv1/cache/serv1-hello/logo*
	rm -fr /var/cache/mediatex/serv1/cache/serv1-hello/supports
	reloadInitdScript
    else
	topo "Cleanup"
	rm -fr /var/cache/mediatex/serv1/cache/serv1-hello/incoming
	sed -i -e "/(INC/, /)/ d" /etc/mediatex/serv1-hello/extract000.txt
	reloadInitdScript
    fi
}

# Configure server 2 (as a private network gateway)
function test7()
{
    if [ "x$1" != "xclean" ]; then
	topo "Configure server 2 (as a private network gateway)"
	mdtxA "adm init" serv2
	sedInPlace "s/6561/6002/" /etc/mediatex/serv2.conf
	sedInPlace "s/networks   www/networks   www, private/" \
	    /etc/mediatex/serv2.conf
	echo "Gateways private" >> /etc/mediatex/serv2.conf
	startInitdScript serv2
	finalQuestion "is 2nd server running ?" "statusInitdScript serv2"
    else
	topo "Cleanup"
	stopInitdScript serv2
	mdtxA "adm purge" serv2
    fi
}

# Register collection 'hello' on server 2
function test8()
{
    if [ "x$1" != "xclean" ]; then
	topo "Register collection 'hello' on server 2"

	# create serv2-hello key
	notice "you will be ask for this fingerprint" \
	       "grep 'host fingerprint' /etc/mediatex/serv1.conf"
	mdtxA "adm add coll serv1-hello" serv2

	# register serv2-hello key on serv1
	cp /var/cache/mediatex/serv2/home/serv2-hello/.ssh/id_dsa.pub \
	    /tmp/key
	mdtxP "add key /tmp/key to coll hello" serv1

	# server 2 register for collection hello on server 1
	mdtxA "adm add coll serv1-hello" serv2

	# workaround for multiple server instances (already done else)
	restartInitdScript serv2
	
	question "connected to serv1 cvsroot ?" \
		 "cat /var/cache/mediatex/serv2/cvs/serv2-hello/CVS/Root"
	[ $TEST_OK -eq 0 ] && return
	
	# seen by first server
	mdtxP "upgrade" serv1
	finalQuestion "does 1st server see 2nd server ?" \
	    "grep -A2 Server /var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
    else
	topo "Cleanup"

	# remove serv2-hello keys from serv1
	SRV="/var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
	FPS=$(grep Server $SRV | awk '{ print $2 }')
	for FP in $FPS; do 
	    mdtxP "del key $FP from coll hello" serv1
	done
	
	mdtxA "adm del coll hello" serv2
    fi
}

# First server notify to second one you ask for an archive record not yet available
function test9()
{
    if [ "x$1" != "xclean" ]; then
	topo "Serv1 notify serv2 you ask for an archive record not yet available"

	# same as test4
	yourMail
	mdtxP "srv save" serv1
	question "do the 1st server get it ?" \
	     "cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
	[ $TEST_OK -eq 0 ] && return

	# tell serv2 we need to extract a record
	mdtxP "srv notify" serv1
	mdtxP "srv save" serv2
	finalQuestion "does 2nd server get your demand ?" \
		      "cat /var/cache/mediatex/serv2/md5sums/serv2-hello.md5"
    else
	topo "Cleanup"
	for SERV in serv1 serv2; do
	    stopInitdScript $SERV
	    rm -f /var/cache/mediatex/$SERV/md5sums/$SERV-hello.md5
	    startInitdScript $SERV
	done
    fi
}

# server 2 provides first support and tell it to server 1
function test10()
{
    if [ "x$1" != "xclean" ]; then
	topo "server 2 provides first support and tell it to server 1"
	mdtxP "add supp iso1 on /usr/share/mediatex/misc/logoP1.iso" serv2
	mdtxP "add supp iso1 to coll hello" serv2
	mdtxP "check supp iso1 on /usr/share/mediatex/misc/logoP1.iso" serv2
	mdtxP "srv notify" serv2

	# check server 1 was notified
	mdtxP "srv save" serv1
	question "do the 2nd server notifies it gets materials ?" \
		 "cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
	[ $TEST_OK -eq 0 ] && return
	
	# serv1 scp the remote content
	mdtxP "srv extract" serv1
	mdtxP "srv save" serv1
	finalQuestion "part1 remotely copied by serv1 ?" \
		      "cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
    else
	topo "Cleanup"
	mdtxP "del supp iso1" serv2
	for SERV in serv1 serv2; do
	    rm -f /var/cache/mediatex/$SERV/cache/$SERV-hello/logo*
	    reloadInitdScript $SERV
	done
	mdtxP "srv notify" serv2
    fi
}

# Configure server 3 (from inside a private network)
# (close to test 7)
function test11()
{
    if [ "x$1" != "xclean" ]; then
        topo "Configure server 3 (from inside a private network)"

	mdtxA "adm init" serv3
	sedInPlace "s/6561/6003/" /etc/mediatex/serv3.conf
	sedInPlace "s/networks   www/networks   private/" \
	    /etc/mediatex/serv3.conf
	startInitdScript serv3
	finalQuestion "is 3rd server running ?" "statusInitdScript serv3"
    else
	topo "Cleanup"
	stopInitdScript serv3
	mdtxA "adm purge" serv3
    fi
}

# Register collection 'hello' on server 3
# (close to test 8)
function test12()
{
    if [ "x$1" != "xclean" ]; then
	topo "Register collection 'hello' on server 3"

	# create serv3-hello key
	notice "you will be ask for this fingerprint" \
	       "grep 'host fingerprint' /etc/mediatex/serv1.conf"
	mdtxA "adm add coll serv1-hello" serv3

	# register serv3-hello key on serv1
	cp /var/cache/mediatex/serv3/home/serv3-hello/.ssh/id_dsa.pub /tmp/key
	mdtxP "add key /tmp/key to coll hello" serv1

	# server 3 register for collection hello on server 1
	mdtxA "adm add coll serv1-hello" serv3

	# workaround for multiple server instances (already done else)
	restartInitdScript serv3
	
	question "connected to serv1 cvsroot ?" \
		 "cat /var/cache/mediatex/serv3/cvs/serv3-hello/CVS/Root"
	[ $TEST_OK -eq 0 ] && return
	
	# seen by first servers
	mdtxP "upgrade" serv1
	finalQuestion "does 1st server see 3rd server ?" \
		      "grep -A2 Server 
                        /var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
    else
	topo "Cleanup"

	# remove serv3-hello keys from serv1 (but not serv2 key!)
	SRV="/var/cache/mediatex/serv1/cvs/serv1-hello/servers.txt"
	FPS1=$(grep -B3 end $SRV | grep Server | awk '{ print $2 }')
	FPS2=$(grep -B4 6003 $SRV | grep Server | awk '{ print $2 }')
	for FP in $FPS1 $FPS2; do 
	    mdtxP "del key $FP from coll hello" serv1
	done
	
	mdtxA "adm del coll hello" serv3
    fi
}

# First server notify to third one you ask for an archive record not yet available
# (close to test 9)
function test13()
{
    if [ "x$1" != "xclean" ]; then
	topo "Serv1 notify serv3 (via serv2) you ask for an archive record"
	mdtxP "upgrade" serv2
	mdtxP "srv notify" serv1
	question "please check the logs" \
		 "tail -n 40 /var/log/mediatex.log | grep -i notify"
	[ $TEST_OK -eq 0 ] && return
	
	mdtxP "srv save" serv3
	finalQuestion "does 3rd server get your demand ?" \
		      "cat /var/cache/mediatex/serv3/md5sums/serv3-hello.md5"
    else
	topo "Cleanup"
	stopInitdScript serv3
	rm -f /var/cache/mediatex/serv3/md5sums/serv3-hello.md5
	startInitdScript serv3
    fi
}

# Server 3 provides second support and tell it to others
# (close to test 10)
function test14()
{
    if [ "x$1" != "xclean" ]; then
	topo "Server 3 provides second support and tell it to others"
 
	mdtxP "add file /usr/share/mediatex/misc/logoP2.iso" serv3
	mdtxP "add supp /usr/share/mediatex/misc/logoP2.iso to coll hello" serv3
	mdtxP "srv extract" serv3
	mdtxP "srv save" serv3
	question "does serv3 get part2 ?" \
		 "cat /var/cache/mediatex/serv3/md5sums/serv3-hello.md5"
	[ $TEST_OK -eq 0 ] && return
	
	mdtxP "srv notify" serv3
	mdtxP "srv extract" serv2
	mdtxP "srv save" serv2
	finalQuestion "part2 remotely copied by serv2 ? (and mediatex.css too)" \
		      "cat /var/cache/mediatex/serv2/md5sums/serv2-hello.md5"
    else
	topo "Cleanup"
	mdtxP "del supp /usr/share/mediatex/misc/logoP2.iso" serv3
	for SERV in serv1 serv2; do
	    rm -f /var/cache/mediatex/$SERV/cache/$SERV-hello/logoP2.iso
	    rm -f /var/cache/mediatex/$SERV/cache/$SERV-hello/logoP2.cat
	    rm -f /var/cache/mediatex/$SERV/cache/$SERV-hello/logo.tgz
	    rm -fr /var/cache/mediatex/$SERV/cache/$SERV-hello/logo
	    reloadInitdScript $SERV
	done
	rm -fr /var/cache/mediatex/serv3/cache/serv3-hello/logo*
	reloadInitdScript serv3
    fi
}

# Server 1 retrieve the archive and notify other it is no more looking for
function test15()
{
    if [ "x$1" != "xclean" ]; then
	topo "Server 1 retrieve the archive and notify other it is no more looking for"
 
	mdtxP "srv notify" serv2
	mdtxP "srv extract" serv1
	mdtxP "srv save" serv1
	question "does serv1 deliver logo.png (mail sent) ?" \
		 "cat /var/cache/mediatex/serv1/md5sums/serv1-hello.md5"
	[ $TEST_OK -eq 0 ] && return
	
	mdtxP "srv notify" serv1
	mdtxP "srv save" serv2
	mdtxP "srv save" serv3
	finalQuestion "does serv2 and serv3 are no more looking for logo.png ?" \
		 "cat /var/cache/mediatex/serv2/md5sums/serv2-hello.md5 \
                      /var/cache/mediatex/serv3/md5sums/serv3-hello.md5"
	[ $TEST_OK -eq 0 ] && return
    else
	topo "Cleanup"
	rm -f /var/cache/mediatex/serv1/cache/serv1-hello/logo.png
	reloadInitdScript serv1
    fi
}

# Move CVS repository from serv1 to serv2
function test16()
{
    if [ "x$1" != "xclean" ]; then
	topo "Move CVS repository from serv1 to serv2"
	if [ -d /var/lib/mediatex/serv1/serv1-hello ]; then
	    cp -fr /var/lib/mediatex/serv1/serv1-hello /tmp/test15
	    mv /var/lib/mediatex/serv1/serv1-hello \
	       /var/lib/mediatex/serv2/serv2-hello
	fi
	
	mdtxA "adm add coll hello" serv2
	question "connected to serv2 cvsroot ?" \
		 "cat /var/cache/mediatex/serv2/cvs/serv2-hello/CVS/Root"
	[ $TEST_OK -eq 0 ] && return

	notice "you will be ask for localhost fingerprint..." \
	       "grep 'host fingerprint' /etc/mediatex/serv1.conf"
	mdtxA "adm add coll serv2-hello" serv1
	question "connected to serv2 cvsroot ?" \
		 "cat /var/cache/mediatex/serv1/cvs/serv1-hello/CVS/Root"
	[ $TEST_OK -eq 0 ] && return

	notice "you will be ask for localhost fingerprint..." \
	       "grep 'host fingerprint' /etc/mediatex/serv1.conf"
	mdtxA "adm add coll serv2-hello" serv3
	finalQuestion "connected to serv2 cvsroot ?" \
		      "cat /var/cache/mediatex/serv3/cvs/serv3-hello/CVS/Root"
	[ $TEST_OK -eq 0 ] && return
	
	rm -fr /var/lib/mediatex/serv1/serv1-hello
    else
	topo "Cleanup"
	if [ -d /tmp/test15 ]; then
	    rm -fr /var/lib/mediatex/serv2/serv2-hello
	    mv /tmp/test15 /var/lib/mediatex/serv2/serv2-hello /tmp
	fi
    fi
}

# Make all functionnal for manuals tests
function test17()
{
    if [ "x$1" != "xclean" ]; then
	topo "Make all functionnal for manuals tests"
	
	for SERV in serv1 serv2; do
	    mdtxP "upgrade+" $SERV
	done
	finalQuestion "All tests done ! Do you want to clean all ?"
	[ $TEST_OK -eq 0 ] && return
	cleanAll
	exit 0	
    else
	topo "Do not clean (you can run './tests.sh clean' to do it latter)"
    fi
}

############################################
# main function
############################################

# check being root
[ $(id -u) -ne 0 ] && {
    echo "** you need to be root for this test"
    exit 1
}

# check no hello users are logged
ps -ef | grep h[e]llo
[ $? -ne 1 ] && {
    echo "** Please logout hello users"
    who | grep hello
    exit 1
}

# cleaning to restart tests from scratch
if [ "x$1" = "xclean" ]; then
    cleanAll
    exit 0
fi

# get the last failling test
TEST_OK=1
TEST_NB=1
[ -f /tmp/test.nb ] && TEST_NB=$(cat /tmp/test.nb)

# run tests
while [ $TEST_OK -eq 1 ]; do
    echo $TEST_NB > /tmp/test.nb
    test$TEST_NB
done

# cleaning before exiting
if [ $TEST_OK -eq 0 ]; then
    # clean last failing test
    test$TEST_NB clean
fi

