#!/bin/bash

# usage of script :
# replay.sh TEST_NAME=generic_test TEST_FOLDER=tst EXECUTABLE=build/vmp RESOURCES=res

EXECUTABLE=build/vmp
TEST_NAME=generic_test
TEST_FOLDER=tst
RESOURCES=res
FRAME=1200x800
FONT=svg/terminus.otb
ORGANIZE=true

# parse arguments

echo "REPLAY"

for ARGUMENT in "$@"
do
   KEY=$(echo $ARGUMENT | cut -f1 -d=)
   KEY_LENGTH=${#KEY}
   VALUE="${ARGUMENT:$KEY_LENGTH+1}"
   export "$KEY"="$VALUE"
   echo "$KEY"="$VALUE"
done

if [[ "$TEST_NAME" == *"non_organized"* ]];then
   ORGANIZE=false
fi
   
SOURCE_LIB="$TEST_FOLDER/test_library"
MASTER_LIB="$TEST_FOLDER/$TEST_NAME/library_master"
TARGET_LIB="$TEST_FOLDER/$TEST_NAME/library_test"
SESSION="$TEST_FOLDER/$TEST_NAME/session.rec"

rm -rf $TARGET_LIB
cp -r $SOURCE_LIB $TARGET_LIB 

echo "REPLAYING $TEST_NAME"
echo "COMMAND: $EXECUTABLE \
-v \
--resources=$RESOURCES \
--replay=$SESSION \
--library=$TARGET_LIB \
--frame=1200x800 \
--font=$FONT \
--organize=$ORGANIZE"

$EXECUTABLE \
    -v \
    --resources=$RESOURCES \
    --replay=$SESSION \
    --library=$TARGET_LIB \
    --frame=1200x800 \
    --font=$FONT \
    --organize=$ORGANIZE

echo "REPLAYING FINISHED"

# remove empty dirs from both directiories to make diff work

find $MASTER_LIB -empty -type d -delete
find $TARGET_LIB -empty -type d -delete

diff -r $MASTER_LIB $TARGET_LIB

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
