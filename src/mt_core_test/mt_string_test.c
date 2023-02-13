#ifndef mt_string_test_h
#define mt_string_test_h

void mt_string_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_string.c"

void mt_string_test_main()
{
    char* str = "000 Állítólag svájcban";

    /* checking if mt_string_new_format works */

    mt_log_debug("testing mt_string_new");

    char* str1 = mt_string_new_format(100, "%s-%i-%x", "milcsi", 100, 64);

    assert(strcmp(str1, "milcsi-100-40") == 0);
}

#endif
