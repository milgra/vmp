#!/bin/bash

exe="$1/vmp"

#tst/test_rec.sh tst/ui_delete build/vmp -o
#tst/test_rep.sh tst/ui_delete build/vmp -o
#build/vmp -r res -v -s tst/ui_delete_test/record -l tst/ui_delete_test -c tst/ui_delete_test -f 1200x800 -o
#build/vmp -r res -v -p tst/ui_delete_test/record -l tst/ui_delete_test -c tst/ui_delete_test -f 1200x800 -o

echo "Test metadata editing in a non-organized library"
sh tst/test_rep.sh tst/ui_non_organized_meta $exe

echo "Test metadata editing in an organized library"
sh tst/test_rep.sh tst/ui_organized_meta $exe -o

echo "Test delete"
sh tst/test_rep.sh tst/ui_delete $exe -o

error=$?
if [ $error -eq 0 ]
then
    echo "No differences found between master and result images"
    exit 0
elif [ $error -eq 1 ]
then
    echo "Differences found between master and result images"
    exit 1
else
    echo "Differences found between master and result images"
    exit 1
fi
