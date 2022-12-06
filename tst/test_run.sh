#!/bin/bash

exe="$1/vmp"

#tst/test_rec.sh tst/ui_delete build/vmp -o
#tst/test_rep.sh tst/ui_delete build/vmp -o
#build/vmp -r res -v -s tst/ui_delete_test/record -l tst/ui_delete_test -c tst/ui_delete_test -f 1200x800 -o
#build/vmp -r res -v -p tst/ui_delete_test/record -l tst/ui_delete_test -c tst/ui_delete_test -f 1200x800 -o

echo -e "Test metadata editing in a non-organized library\n\n\n"
sh tst/test_rep.sh tst/meta_non_organized $exe

echo -e "Test metadata editing in an organized library\n\n\n"
sh tst/test_rep.sh tst/meta_organized $exe -o

echo -e "Test non-organized delete\n\n\n"
sh tst/test_rep.sh tst/delete_non_organized $exe

echo -e "Test organized delete\n\n\n"
sh tst/test_rep.sh tst/delete_organized $exe -o

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
