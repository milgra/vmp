#ifndef mt_vector_test_h
#define mt_vector_test_h

void mt_vector_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_vector.c"

void mt_vector_test_main()
{
    char* text1 = "Test text 1";
    char* text2 = "Test text 2";

    char* ttext1 = mt_memory_stack_to_heap(strlen(text1) + 1, NULL, NULL, (char*) text1);
    char* ttext2 = mt_memory_stack_to_heap(strlen(text2) + 1, NULL, NULL, (char*) text2);

    /* checking if vector allocated correctly */

    mt_log_debug("testing mt_vector_new");

    mt_vector_t* vec1 = mt_vector_new();

    assert(vec1 != NULL);
    assert(vec1->length == 0);

    /* checking if add works correctly */

    mt_log_debug("testing mt_vector_add");

    mt_vector_add(vec1, ttext1);

    assert(vec1->length == 1);
    assert(vec1->data[0] == ttext1);

    /* checking if add release works correctly */

    mt_log_debug("testing mt_vector_add_rel");

    mt_vector_add_rel(vec1, ttext2);

    assert(vec1->length == 2);
    assert(vec1->data[1] == ttext2);
    assert(mt_memory_retaincount(ttext2) == 1);

    /* checking if add unique works correctly */

    mt_log_debug("testing mt_vector_add_unique_data");

    mt_vector_add_unique_data(vec1, ttext1);

    assert(vec1->length == 2);
    assert(vec1->data[0] == ttext1);
    assert(vec1->data[1] == ttext2);
    assert(mt_memory_retaincount(ttext1) == 2);

    /* checking if add in vector works correctly */

    mt_log_debug("testing mt_vector_add_unique_data");

    /* checking if insert works correctly */

    mt_log_debug("testing mt_vector_ins");

    /* checking if insert unique data works correctly */

    mt_log_debug("testing mt_vector_ins_unique");

    /* checking if remove data works correctly */

    mt_log_debug("testing mt_vector_rem");

    mt_vector_rem(vec1, ttext1);
}

#endif
