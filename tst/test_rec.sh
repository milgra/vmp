#!/bin/bash

# first copy start folder structure to test folder

if [ $1 == 0 ];
then
echo "COPYING start TO test"
rm -r tst/test
rm -r tst/result
cp -r tst/start tst/test
fi

# execute record session on test folder

echo "STARTING RECORDING SESSION" $1

cnt=$1
res="y"

while [ $res = "y" ]; do

    res_path="../res"
    ses_path="../tst/session$cnt.rec"
    lib_path="../tst/test"
    frm_size="1000x900"
    
    ((cnt+=1))
    
    build/vmp -r $res_path -s $ses_path -f $frm_size -l $lib_path

    echo "Record another session? y/n"

    read res

done

echo "RENAMING test to result"

mv tst/test tst/result

echo "RECORDING FINISHED"
