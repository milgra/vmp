#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    # move stuff to testdir so all path remain the same during testing and recording
    basedir="$1_base"
    testdir="$1_test"
    savedir="$1_test/record"
    masterdir="$1_master"
    rm -rf $testdir
    cp -r $basedir $testdir 
    echo "(RE-)RECORDING $1"
    echo "COMMAND: build/vmp -r res -v -p $savedir -l $testdir -c $savedir -f 1200x800 $3"
    build/vmp -r res -v -s $savedir -l $testdir -c $savedir -f 1200x800 $3
    echo "RECORDING FINISHED"
    rm -rf $masterdir
    cp -r $testdir $masterdir
    rm -rf $testdir
fi
