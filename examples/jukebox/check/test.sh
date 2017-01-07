#!/bin/bash
#set -x
set -e

# retrieve environment
[ -z $srcdir ] && srcdir=.

mkdir -p tmp
TEST=$1

# add mediatex inputs
for EXT in motd supp; do
    sed $srcdir/$TEST.$EXT \
	-e "s,{SRC},$srcdir," \
	-e "s,{BLD},$PWD," \
	> tmp/$TEST.$EXT
done

# add extraction rules if not already there
if [ ! -f tmp/$TEST.extr ]; then
    cp $srcdir/$TEST.extr tmp/$TEST.extr
fi

# call jukebox
$srcdir/../bin/jukebox.sh \
    -v -n -d ./tmp \
    -f $srcdir/$TEST.conf -r tmp/$TEST.extr \
    -m tmp/$TEST.motd -s tmp/$TEST.supp \
    > tmp/$TEST.out 2>&1

# check output
SRCDIR=$(echo "$srcdir" | sed -e 's,\.,\\.,g')
sed tmp/$TEST.out \
    -e "s,$SRCDIR/,{SRC}/,g" \
    -e "s,$PWD/,{BLD}/,g" \
    -e "s,[0-9a-f]\{32\}:[0-9]\+,{HASH}:{SIZE},g" \
    -e "s,\(iso size estimated to\) [0-9]\+,\1 {SIZE},g" \
    > $TEST.out

diff $srcdir/$TEST.exp $TEST.out
