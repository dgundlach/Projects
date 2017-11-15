#!/bin/sh
OFS=$IFS
IFS=":"
cat /etc/passwd | while read p1 p2 p3 p4 p5 p6 p7 ; do
echo $p1 $p6
done
IFS=$OFS
echo :$IFS:

