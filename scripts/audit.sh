#!/bin/bash
#set -x
set -e
#=======================================================================
# * Version: $Id: audit.sh,v 1.3 2015/09/22 23:05:55 nroche Exp $
# * Project: MediaTex
# * Module : scripts
# *
# * This script manage the audit report file
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

[ -z $srcdir ] && srcdir=.
[ -z $libdir ] && libdir=$srcdir/lib
[ ! -z $MDTX_SH_INCLUDE ] || source $libdir/include.sh
[ ! -z "$MDTX_MDTXUSER" ] || 
Error "expect MDTX_MDTXUSER variable to be set by the environment"
[ ! -z $1 ] || Error "expect a label as first parameter"
[ ! -z $2 ] || Error "expect an extra record field as second parameter"
[ ! -z $3 ] || Error "expect a hash as third parameter"
[ ! -z $4 ] || Error "expect a size as fourth parameter"
[ ! -z $5 ] || Error "expect a status as fifth parameter"

Notice "audit: archive $3:$4 for coll $1 (status = $5)"

COLL=$1
EXTRA=$2
HASH=$3
SIZE=$4
STATUS=$5

USER="$MDTX-$COLL"
AUDIT=${EXTRA/:*/}
ADDRESS=${EXTRA/*:/}

FILENAME=${AUDIT/:*/}".txt"
FILENAME_OK=${AUDIT/:*/}"_ok.txt"
FILENAME_KO=${AUDIT/:*/}"_ko.txt"
FILEPATH=$EXTRACT/$USER/$FILENAME
FILEPATH_OK=$EXTRACT/$USER/$FILENAME_OK
FILEPATH_KO=$EXTRACT/$USER/$FILENAME_KO
NAME=$(echo $ADDRESS | cut -d "@" -f1)
SUBJECT="$USER audit"

# # check audit report is still available
# # (many many mails will be sent!!)
# if [ ! -f $FILEPATH ]; then

#     # send mail
#     /usr/bin/mail $ADDRESS -s "$SUBJECT" <<EOF
# Dear $NAME,

# The audit you requested on $COLL collection fails.
# - $EXTRA

# This do not implies your collection leaks,
# but only that the audit process itself do not complete.

# Sorry about that.

# $([ ! -f /usr/games/cowsay ] || /usr/games/cowsay mheu)
# EOF
#      Warning "audit for $ADDRESS uncompleted"
#      exit 0;
# fi
     
# retrieve actual values from audit file
MAX=$(head -n4 $FILEPATH | tail -n +4 | cut -d' ' -f2)
CUR=$(head -n5 $FILEPATH | tail -n +5 | cut -d' ' -f2)
OK=$(head -n6 $FILEPATH | tail -n +6 | cut -d' ' -f3)
KO=$(head -n7 $FILEPATH | tail -n +7 | cut -d' ' -f3)

# assert archive was not already checked
if [ "$(grep -c $HASH:$SIZE $FILEPATH)" -ne 1 ]; then
    Info "archive $HASH:$SIZE already audited for $EXTRA"
else
    # if check on archive was successful
    if [ $STATUS -eq $TRUE ]; then
	echo " $HASH:$SIZE $(date)" >> $FILEPATH_OK
	OK=$[$OK + 1]
    else
	echo " $HASH:$SIZE $(date)" >> $FILEPATH_KO
	KO=$[$KO + 1]
    fi

    # archive now checked
    CUR=$[$CUR + 1]

    sed -i $FILEPATH \
	-e "5 s/[0-9]\+/$CUR/" \
	-e "6 s/[0-9]\+/$OK/" \
	-e "7 s/[0-9]\+/$KO/" \
	-e "/$HASH:$SIZE/ d"

    # finalise the audit repport    
    if [ $CUR -eq $MAX ]; then
	echo -e "\nArchive OK:" >> $FILEPATH
	if [ -e $FILEPATH_OK ]; then
	    cat $FILEPATH_OK >> $FILEPATH
	else
	    echo " none" >> $FILEPATH
	fi
	echo -e "\nArchive KO:" >> $FILEPATH
	if [ -e $FILEPATH_KO ]; then
	    cat $FILEPATH_KO >> $FILEPATH
	else
	    echo " none" >> $FILEPATH
	fi
	rm -f $FILEPATH_OK $FILEPATH_KO
    fi
fi

# upload the audit report and send a mail
if [ $CUR -eq $MAX ]; then
    DO_LOG=$(grep -i logAudit $CVSCLT/$USER/servers.txt | \
		    awk '{ print $2 }')
    case $DO_LOG in
	yes|Yes|YES|y|Y|true|True|TRUE|t|T)
	    SIGN1=$(md5sum $FILEPATH | cut -d' ' -f 1)
	    SIGN1=$SIGN1:$(ls $FILEPATH -l | cut -d' ' -f 5)
	    
	    gzip -c $FILEPATH > $FILEPATH.gz
	    SIGN2=$(md5sum $FILEPATH.gz | cut -d' ' -f 1)
	    SIGN2=$SIGN2:$(ls $FILEPATH.gz -l | cut -d' ' -f 5)
	
	    cat >/tmp/$AUDIT.cat <<EOF
Top Category "~mediatex"
Category "audits": "~mediatex"
Document "$AUDIT": "audits"
 "requested on" = "$(date)"
 "requested by" = "$ADDRESS"
 "requested from" = "$(hostname -f)"
 "archive checked" = "$CUR"
$SIGN1
$SIGN2

Archive $SIGN1
 "format" = "text"
Archive $SIGN2
 "format" = "gz"
EOF
    
	    TARGETDIR="mediatex/audits"
	    cat >/tmp/$AUDIT.ext <<EOF
(GZIP
$SIGN2
=>
$SIGN1 $TARGETDIR/$FILENAME
)
EOF

	    mediatex -c $MDTX upload++ file $FILEPATH.gz as $TARGETDIR/ \
		     catalog /tmp/$AUDIT.cat rules /tmp/$AUDIT.ext \
		     to coll $COLL
	    rm -f /tmp/$AUDIT.cat /tmp/$AUDIT.ext $FILEPATH.gz
	    ;;
    esac
    mv $FILEPATH /tmp/$FILENAME

    # send mail
    /usr/bin/mail $ADDRESS -s "$SUBJECT" <<EOF
Dear $NAME,

The audit you requested on $COLL collection is completed.
- /tmp/$FILENAME
- upload: $DO_LOG

$([ ! -f /usr/games/cowsay ] || /usr/games/cowsay mheu)
EOF
     Notice "audit for $ADDRESS completed"
fi

Info "done"
