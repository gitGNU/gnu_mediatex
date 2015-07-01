#!/bin/bash
echo -e "Content-type: text/html\r\n"
echo -e "\r\n"

echo "<h1>env</h1>"
for i in $(env)
do
	echo -e "$i<br>\r\n"
done

echo "<h1>stdin</h1>"
rc=0
while [ $rc -eq 0 ]
do
        read -t2 l
        rc=$?
        echo -e "$l<br>\r\n"
done

