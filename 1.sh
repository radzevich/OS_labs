#!/bin/bash

counter=0
ERROR_LOG="error.log"
IFS=$'\n'
exec 2<> $ERROR_LOG

if [ -e "$3" ] 
then
	rm "$3"
fi

counter=0
for file in $(find "$4" -size +$1c -size -$2c -type f)
do
	((counter++))
	printf "%s  %s  %s\n" $(dirname "$file") $(basename "$file") $(stat -c %s "$file") >> "$3"
done
echo $counter

cat error.log | while read line
do
	echo "$1.sh: $line"
done
exec $ERROR_LOG>&-

exit 0
