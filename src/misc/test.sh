#/bin/bash

rc=0
while [ $rc -eq 0 ]
do
	read -t2 l
	rc=$?
	echo $l
done
