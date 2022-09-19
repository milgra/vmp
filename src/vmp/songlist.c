#ifndef songlist_h
#define songlist_h

char* songlist_get_current_path();
char* songlist_get_prev_path();
char* songlist_get_next_path();
void  songlist_toggle_shuffle();

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdlib.h>

char* songlist_get_current_path()
{
    return NULL;
}

char* songlist_get_prev_path()
{
    return NULL;
}

char* songlist_get_next_path()
{
    return NULL;
}

void songlist_toggle_shuffle()
{
}

#endif
