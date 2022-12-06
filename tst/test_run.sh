#!/bin/bash

exe="$1/vmp"

#tst/test_rec.sh tst/ui_delete build/vmp -o
#tst/test_rep.sh tst/ui_delete build/vmp -o
#build/vmp -r res -v -s tst/ui_delete_test/record -l tst/ui_delete_test -c tst/ui_delete_test -f 1200x800 -o
#build/vmp -r res -v -p tst/ui_delete_test/record -l tst/ui_delete_test -c tst/ui_delete_test -f 1200x800 -o

echo -e "\nTest metadata editing in a non-organized library\n"
sh tst/test_rep.sh tst/meta_non_organized $exe

echo -e "\nTest metadata editing in an organized library\n"
sh tst/test_rep.sh tst/meta_organized $exe -o

echo -e "\nTest non-organized delete\n"
sh tst/test_rep.sh tst/delete_non_organized $exe

echo -e "\nTest organized delete\n"
sh tst/test_rep.sh tst/delete_organized $exe -o

echo -e "\nTest organized play\n"
sh tst/test_rep.sh tst/play_organized $exe -o

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
