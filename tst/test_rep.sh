#!/bin/bash

# usage of script :
# ./tst/test_rep.sh tst/delete_non_organized build/vmp

# $1 - test folder name
# $2 - executable
# $3 - organize

# to record a session :
# cp -r "tst/test_library" "tst/delete_non_organized/library_master" 
# vmp --replay="tst/tdelete_non_organized/session.rec" \
#     --library="tst/delete_non_organized/library_master" \
#     --organize

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    
    source_library="tst/test_library"
    master_library="$1/library_master"
    session_library="$1/library_test"
    session_file="$1/session.rec"

    rm -rf $session_library
    cp -r $source_library $session_library 

    echo "REPLAYING $1"    
    echo "COMMAND: $1 --resources=res --replay=$session_file --library=$session_library -frame=1200x800 $3"

    $2 --resources=res \
       --replay=$session_file \
       --library=$session_library \
       --frame=1200x800 \
       $3
    
    echo "REPLAYING FINISHED"

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
