#!/bin/bash
#set -x

FILE=download
BASE=open_icon_library-standard

function addArchive()
{
    FILE_MD5=$(md5sum $1/$2 | cut -d' ' -f 1)
    FILE_SIZE=$(ls $1/$2 -l | cut -d' ' -f 5)

    cat >> icons.cat<<EOF
Document "${2/.$FORMAT/}": "$3"
 $FILE_MD5:$FILE_SIZE

Archive $FILE_MD5:$FILE_SIZE
 "format" = "$FORMAT"
 "size" = "$SIZE"
EOF
}

if [ ! -f $FILE ]; then
    echo "* download openiconlibrary"
    wget http://sourceforge.net/projects/openiconlibrary/files/latest/download
fi

if [ ! -d $BASE ]; then
    echo "* extract openiconlibrary"
    tar -jxf download
fi

if [ ! -f icons.ext ]; then
    echo "* build extraction rules"

    if [ ! -f icons.tmp ]; then
	find $BASE -type f -exec sh -c \
	     'FILE_MD5=$(md5sum {} | cut -d" " -f 1);
     FILE_SIZE=$(ls -l {} | cut -d" " -f 5);
     echo "$FILE_MD5:$FILE_SIZE {}" >> icons.tmp' \;
    fi
	
    FILE_MD5=$(md5sum $FILE | cut -d' ' -f 1)
    FILE_SIZE=$(ls $FILE -l | cut -d' ' -f 5)
    
    cat > icons.ext <<EOF
(TBZ
$FILE_MD5:$FILE_SIZE
=>
EOF
    
    sort icons.tmp -k 1,1 -u >> icons.ext 
    echo ")" >> icons.ext
fi

if [ ! -f icons.cat ]; then
    echo "* build catalog"

    for FORMAT in ico png; do
	echo "** $FORMAT"
	DIR_FORMAT=$BASE/icons/$FORMAT

	for SIZE in $(ls $DIR_FORMAT); do
	    echo "*** $SIZE"
	    DIR_SIZE=$BASE/icons/$FORMAT/$SIZE

	    for TYPE1 in $(ls $DIR_SIZE); do
		echo "**** $TYPE1"
 		DIR_TYPE1=$BASE/icons/$FORMAT/$SIZE/$TYPE1

		cat >> icons.cat<<EOF
Top Category "$TYPE1"
EOF

		# simple type icone
		for FILE in $(ls $DIR_TYPE1 | grep "\.$FORMAT\$"); do
	    	    addArchive $DIR_TYPE1 $FILE $TYPE1
		done

		for DIR_TYPE2 in $(find $DIR_TYPE1 -mindepth 1 -maxdepth 1 -type d); do
		    TYPE2=$(basename $DIR_TYPE2)
		    echo "***** $TYPE2"

		    cat >> icons.cat<<EOF
Category "$TYPE2": "$TYPE1"
EOF

	            # double type icone
		    for FILE in $(ls $DIR_TYPE2 | grep "\.$FORMAT\$"); do
			addArchive $DIR_TYPE2 $FILE $TYPE2
		    done
		done
	    done
	done
    done
fi
