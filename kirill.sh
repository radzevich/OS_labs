#!/bin/bash
ERR_LOG="/tmp/err.log"
exec 6>&2 2>$ERR_LOG
current=$(pwd)
if [ -s $ERR_LOG ]
then
    exit 0 
fi
find $1 -size -$2c -a -size +$3c -exec du -c {} + | ( tail -n 1; ls | wc -l; du -a -h --total --max-depth=40 $1 ) 

exec 2>&6 6>&-
sed "s/.[a-zA-Z ]*:/`basename $0`:/" < $ERR_LOG 1>&2

