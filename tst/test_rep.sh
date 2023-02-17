#!/bin/bash

# usage of script :
# ./tst/test_rep.sh build/vmp tst/test_library tst/delete_non_organized res 

# $1 - executable
# $2 - test library source
# $3 - test folder name
# $4 - resources folder
# $5 - organize

# to record a session :
# cp -r "tst/test_library" "tst/delete_non_organized/library_master" 
# vmp --replay="tst/tdelete_non_organized/session.rec" \
#     --library="tst/delete_non_organized/library_master" \
#     --organize

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    
    source_library=$2
    master_library="$3/library_master"
    session_library="$3/library_test"
    session_file="$3/session.rec"

    rm -rf $session_library
    cp -r $source_library $session_library 

    echo "REPLAYING $3"    
    echo "COMMAND: $1 --resources=$4 --replay=$session_file --library=$session_library -frame=1200x800 $5"

    $1 --resources=$4 \
       --replay=$session_file \
       --library=$session_library \
       --frame=1200x800 \
       $5
    
    echo "REPLAYING FINISHED"

    # remove empty dirs from both directiories to make diff work

    find master_library --empty -type d --delete
    find session_library --empty -type d --delete

    diff -r $master_library $session_library
    
    error=$?
    if [ $error -eq 0 ]
    then
	echo "No differences found between master and test folders"
	exit 0
    elif [ $error -eq 1 ]
    then
	echo "Differences found between master and result folders"
	exit 1
    else
	echo "Differences found between master and result folders"
	exit 1
    fi

fi
