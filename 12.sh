#!/bin/bash
<<<<<<< HEAD
current=$(pwd)
echo "$1" > 1.txt
cd $1
du -a -h --total --max-depth=40  >> $current/1.txt
#echo $size
=======
cd $1
let TOTAL_SIZE=0
for f in *
do
  if [ -d $f ]
  then
    cd $f
    for file in *
    do
	temp=$(stat -c%s $file)
	TOTAL_SIZE=$((TOTAL_SIZE+=$temp))
    done
    cd ..
  fi
done
echo "$TOTAL_SIZE"
exit 0
>>>>>>> 62c25327569df8afe339e90c2dddf776a1b11ffd
