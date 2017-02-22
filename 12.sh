#!/bin/bash
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
