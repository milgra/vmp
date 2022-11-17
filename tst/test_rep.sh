#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    basedir="$1_base"
    testdir="$1_test"
    savedir="$1_test/record"
    masterdir="$1_master"
    echo "REPLAYING $1, TESTDIR $testdir"
    # cleanup
    rm -rf $testdir
    cp -r $basedir $testdir 
    # copy session record
    cp "$masterdir/record/session.rec" "$savedir/"
    cd $savedir
    rm -rf *.kvl
    rm -rf screenshot*
    cd ../../..
    build/vmp -r res -v -p $savedir -l $testdir -c $savedir    
    echo "REPLAY FINISHED, DIFFING"
    diff -r $masterdir $testdir
fi

