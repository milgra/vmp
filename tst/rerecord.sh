#!/bin/bash

#!/bin/bash

# usage of script :
# rerecord.sh TEST_NAME=generic_test TEST_FOLDER=tst EXECUTABLE=build/vmp RESOURCES=res

EXECUTABLE=build/vmp
TEST_NAME=generic_test
TEST_FOLDER=tst
RESOURCES=res
FRAME=1200x800
FONT=svg/terminus.otb
ORGANIZE=true

# parse arguments

for ARGUMENT in "$@"
do
   KEY=$(echo $ARGUMENT | cut -f1 -d=)
   KEY_LENGTH=${#KEY}
   VALUE="${ARGUMENT:$KEY_LENGTH+1}"
   export "$KEY"="$VALUE"
done

if [[ "$TEST_NAME" == *"non_organized"* ]];then
   ORGANIZE=false
fi
   
SOURCE_LIB="$TEST_FOLDER/test_library"
TARGET_LIB="$TEST_FOLDER/$TEST_NAME/library_master"
SESSION="$TEST_FOLDER/$TEST_NAME/session.rec"

rm -rf $TARGET_LIB
cp -r $SOURCE_LIB $TARGET_LIB 

echo "RE-RECORDING $1"    
echo "COMMAND: $EXECUTABLE \
--resources=$RESOURCE \
--replay=$SESSION \
--library=$TARGET_LIB \
--oragnize=$ORGANIZE \
--frame=1200x800 \
--font=$FONT \
-v"

$EXECUTABLE \
    --resources=$RESOURCES \
    --replay=$SESSION \
    --library=$TARGET_LIB \
    --organize=$ORGANIZE \
    --frame=1200x800 \
    --font=$FONT \
    -v

echo "RE-RECORDING FINISHED"

