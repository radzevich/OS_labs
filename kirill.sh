#!/bin/bash
ERR_LOG="/tmp/err.log"
exec 6>&2 2>$ERR_LOG
current=$(pwd)
if [ -s $ERR_LOG ]
then
    exit 0 
fi
min_size=$2
echo "$(readlink -f $1)"
find $1 -type f  -size -$2c -a -size +$3c -exec du -ch {} + | ( grep total )
find $1 -type f  -size -$2c -a -size +$3c -exec du -ch {} + | ( wc -l )
#find $1 -type f -size +$2k -a -size -$3k + | wc -l
exec 2>&6 6>&-
sed "s/.[a-zA-Z ]*:/`basename $0`:/" < $ERR_LOG 1>&2

