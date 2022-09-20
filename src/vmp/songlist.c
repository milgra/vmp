#ifndef songlist_h
#define songlist_h

#include "zc_map.c"
#include "zc_vector.c"

char* songlist_get_current_path();
char* songlist_get_prev_path();
char* songlist_get_next_path();
void  songlist_set_current_index(int index);
void  songlist_toggle_shuffle();
void  songlist_set_songs(vec_t* songs);
void  songlist_set_filter(char* filter);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdlib.h>

struct _songlist_t
{
    int current_index;

    vec_t* songs;         // all items in library
    vec_t* visible_songs; // filtered items
} sl = {0};

char* songlist_get_current_path()
{
    char* path = NULL;
    if (sl.visible_songs && sl.current_index < sl.visible_songs->length)
    {
	map_t* song = sl.visible_songs->data[sl.current_index];
	path        = MGET(song, "path");
    }
    return path;
}

char* songlist_get_prev_path()
{
    char* path = NULL;
    if (sl.visible_songs && sl.current_index - 1 > -1)
    {
	map_t* song = sl.visible_songs->data[sl.current_index - 1];
	path        = MGET(song, "path");
    }
    return path;
}

char* songlist_get_next_path()
{
    char* path = NULL;
    if (sl.visible_songs && sl.current_index + 1 < sl.visible_songs->length)
    {
	map_t* song = sl.visible_songs->data[sl.current_index + 1];
	path        = MGET(song, "path");
    }
    return path;
}

void songlist_set_current_index(int index)
{
    sl.current_index = index;
}

void songlist_toggle_shuffle()
{
}

void songlist_set_songs(vec_t* songs)
{
    if (sl.songs) REL(sl.songs);
    sl.songs = NULL;
    if (songs) sl.songs = RET(songs);
}

void songlist_set_filter(char* filter)
{
}

#endif
