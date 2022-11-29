#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    basedir="$1_base"
    testdir="$1_test"
    savedir="$1_test/record"
    masterdir="$1_master"
    echo "REPLAYING $1, BASEDIR $basedir TESTDIR $testdir SAVEDIR $savedir MASTERDIR $masterdir EXE $2"
    # cleanup
    rm -rf $testdir
    cp -r $basedir $testdir 
    # copy session record
    mkdir -p $savedir
    cp "$masterdir/record/session.rec" "$savedir/"
    cd $savedir
    rm -rf *.kvl
    rm -rf screenshot*
    cd ../../..
    $2 -r res -v -p $savedir -l $testdir -c $savedir    
    echo "REPLAY FINISHED, DIFFING"
    diff -r $masterdir $testdir
fi

