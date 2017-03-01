#!/bin/bash
ERR_LOG="/tmp/err.log"
exec 6>&2 2>$ERR_LOG
current=$(pwd)
if [ -s $ERR_LOG ]
then
    exit 0 
fi
echo "$(date)" >> $3
count=`find $1 -maxdepth $2 -type d | wc -l`
((count=count-1))
for d in $( find $1 -maxdepth $2 -type d); do
      echo "$d :" >> $3
      echo "$(find $d -maxdepth 1 -type f | wc -l)" >> $3
done
  echo "count of wathed dirs is: $count" >> $3
exec 2>&6 6>&-
sed "s/.[a-zA-Z ]*:/`basename $0`:/" < $ERR_LOG 1>&2
