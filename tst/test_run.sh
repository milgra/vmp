#!/bin/bash

exe="$1/vmp"

sh tst/test_rep.sh tst/ui_file $exe
sh tst/test_rep.sh tst/ui_keyboard $exe
sh tst/test_rep.sh tst/ui_mouse $exe

exit 1
