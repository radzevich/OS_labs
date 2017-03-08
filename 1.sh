#!/bin/bash

counter=0

if [ -e "$3" ] 
then
	rm "$3"
fi

counter=0

for file in $(find "$4" -size +$1c -size -$2c -type f)
do
	((counter++))
	printf "Dir: %s \nName: %s \nSize: %s\n\n" $(dirname "$file") $(basename "$file") $(stat -c %s "$file") >> "$3"
done

echo $counter

exit 0
