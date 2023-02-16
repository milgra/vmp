#!/bin/bash

exe="$1/vmp"

for filename in tst/*organized; do
    if [[ "$filename" == *"non_organized"* ]];then
       sh tst/test_rep.sh $filename $exe
    else
       sh tst/test_rep.sh $filename $exe -o
    fi
done

error=$?
if [ $error -eq 0 ]
then
    echo "No differences found between master and result images"
    exit 0
elif [ $error -eq 1 ]
then
    echo "Differences found between master and result images"
    exit 1
else
    echo "Differences found between master and result images"
    exit 1
fi
