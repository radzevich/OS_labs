#!/bin/bash

for file in $(dir -R $1)
do
  echo "$(stat -c%n "$file") $(stat -c%s "$file") $(stat -c%A "$file")"
#$(stat -c%s%A "$file")
done

