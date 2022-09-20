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
void  songlist_apply_filter();
void  songlist_apply_sorting();
void  songlist_set_filter(char* filter);
void  songlist_set_sorting(char* sorting);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "cstr_util.c"
#include "zc_cstring.c"
#include <stdlib.h>

struct _songlist_t
{
    int    current_index; // actual index
    vec_t* songs;         // all items in library
    vec_t* visible_songs; // filtered items
    char*  filter;        // artist is Metallica album is Ride the lightning and genre is not Rock and genre is Metal
    char*  sorting;       // 1 is ascending 0 is descending
    vec_t* sortvec;
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

void songlist_apply_filter()
{
}

void songlist_set_filter(char* filter)
{
}

int songlist_comp_entry(void* left, void* right)
{
    map_t* l = left;
    map_t* r = right;

    for (int index = 0; index < sl.sortvec->length; index += 2)
    {
	char* field = sl.sortvec->data[index];
	int   dir   = atoi(sl.sortvec->data[index + 1]);
	if (dir == 0) dir = -1;

	printf("field %s dir %i\n", field, dir);

	char* la = MGET(l, field);
	char* ra = MGET(r, field);

	if (la && ra)
	{
	    if (strcmp(la, ra) == 0) continue;

	    return dir * strcmp(la, ra);
	}
	else
	{
	    if (la)
		return dir;
	    else if (ra)
		return -1 * dir;
	    else
		return 0;
	}
    }

    return 0;
}

void songlist_apply_sorting()
{
    if (sl.sorting)
    {
	if (sl.sortvec) REL(sl.sortvec);
	sl.sortvec = cstr_split(sl.sorting, " ");
	vec_sort(sl.songs, songlist_comp_entry);
    }
}

void songlist_set_sorting(char* sorting)
{
    if (sl.sorting != NULL) REL(sl.sorting);
    sl.sorting = NULL;
    if (sorting) sl.sorting = cstr_new_cstring(sorting);
}

void songlist_set_songs(vec_t* songs)
{
    if (sl.songs) REL(sl.songs);
    sl.songs = NULL;
    if (songs) sl.songs = RET(songs);

    songlist_apply_filter();
    songlist_apply_sorting();
}

#endif
