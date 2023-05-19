#!/bin/bash

# usage of script :
# run.sh BUILD_PATH=build PROJECT_ROOT=.

BUILD_PATH=build
PROJECT_ROOT=.

# parse arguments

echo "TEST_RUN"

for ARGUMENT in "$@"
do
   KEY=$(echo $ARGUMENT | cut -f1 -d=)
   KEY_LENGTH=${#KEY}
   VALUE="${ARGUMENT:$KEY_LENGTH+1}"
   export "$KEY"="$VALUE"
   echo "$KEY"="$VALUE"
done

EXECUTABLE="$BUILD_PATH/vmp"
RESOURCES="$PROJECT_ROOT/res"
TESTLIB="$PROJECT_ROOT/tst/test_library"

for FOLDER in $PROJECT_ROOT/tst/*organized; do
    echo "FOLDER" $FOLDER
    echo "sh $PROJECT_ROOT/tst/replay.sh TEST_NAME=$(basename $FOLDER) TEST_FOLDER=$PROJECT_ROOT/tst EXECUTABLE=$EXECUTABLE RESOURCES=$$RESOURCES"
    sh $PROJECT_ROOT/tst/replay.sh TEST_NAME=$(basename $FOLDER) TEST_FOLDER=$PROJECT_ROOT/tst EXECUTABLE=$EXECUTABLE RESOURCES=$RESOURCES
    
    error=$?
    if [ $error -eq 0 ]
    then
	echo "No differences found between master and result images"
    elif [ $error -eq 1 ]
    then
	echo "Differences found between master and result images"
	exit 1
    else
	echo "Differences found between master and result images"
	exit 1
    fi
done

exit 0
