#!/bin/bash

# usage of script :
# ./tst/test_rec.sh tst/delete_non_organized build/vmp

# $1 - test folder name
# $2 - executable
# $3 - organize

# to record a session :
# cp -r "tst/test_library" "tst/delete_non_organized/library_master" 
# vmp --record="tst/tdelete_non_organized/session.rec" \
#     --library="tst/delete_non_organized/library_master" \
#     --organize

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    
    source_library="tst/test_library"
    session_library="$1/library_master"
    session_file="$1/session.rec"

    rm -rf $session_library
    cp -r $source_library $session_library 

    echo "RECORDING $1"    
    echo "COMMAND: $1 -v --resources=res --record=$session_file --library=$session_library -frame=1200x800 $3"

    $2 --resources=res \
       --record=$session_file \
       --library=$session_library \
       --frame=1200x800 \
       -v \
       $3
    
    echo "RECORDING FINISHED"

fi
