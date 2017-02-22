#!/bin/bash
current=$(pwd)
echo "$1" > 1.txt
cd $1
du -a -h --total --max-depth=40  >> $current/1.txt
#echo $size
