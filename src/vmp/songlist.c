#ifndef songlist_h
#define songlist_h

#include "zc_map.c"
#include "zc_vector.c"

char*  songlist_get_current_path();
char*  songlist_get_prev_path();
char*  songlist_get_next_path();
void   songlist_set_current_index(int index);
void   songlist_toggle_shuffle();
void   songlist_set_songs(vec_t* songs);
void   songlist_apply_filter();
void   songlist_apply_sorting();
void   songlist_set_filter(char* filter);
void   songlist_set_sorting(char* sorting);
void   songlist_set_fields(map_t* fields);
vec_t* songlist_get_visible_songs();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "cstr_util.c"
#include "utf8.h"
#include "zc_cstring.c"
#include <stdlib.h>

struct _songlist_t
{
    map_t* fields;

    int    current_index; // actual index
    vec_t* songs;         // all items in library
    vec_t* visible_songs; // filtered items

    char*  filter;    // artist is Metallica album is Ride the lightning genre is Rock genre is Metal
    map_t* filtermap; // key - field,
    char*  sorting;   // 1 is ascending 0 is descending
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
    if (sl.visible_songs) vec_reset(sl.visible_songs);
    else sl.visible_songs = VNEW();

    if (sl.filter != NULL)
    {
	vec_t* filtered = VNEW();
	vec_t* fields   = VNEW();

	map_keys(sl.fields, fields);

	for (int index = 0;
	     index < sl.songs->length;
	     index++)
	{
	    map_t* entry = sl.songs->data[index];

	    for (int fi = 0; fi < fields->length; fi++)
	    {
		char* field       = fields->data[fi];
		char* filtervalue = MGET(sl.filtermap, field);
		if (filtervalue == NULL) filtervalue = MGET(sl.filtermap, "*");

		if (filtervalue != NULL)
		{
		    char* value = MGET(entry, field);

		    if (value != NULL)
		    {
			if (utf8casestr(filtervalue, value))
			{
			    VADD(filtered, entry);
			    continue;
			}
		    }
		}
	    }
	}

	vec_add_in_vector(sl.visible_songs, filtered);
	REL(filtered);
    }
    else
    {
	vec_add_in_vector(sl.visible_songs, sl.songs);
    }
}

void songlist_set_filter(char* filter)
{
    if (sl.filter != NULL) REL(sl.filter);
    sl.filter = NULL;
    if (filter) sl.filter = cstr_new_cstring(filter);

    if (sl.filter)
    {
	vec_t* words = cstr_split(sl.filter, " ");

	if (sl.filtermap) REL(sl.filtermap);
	sl.filtermap = MNEW();

	char* currentfield = NULL;
	char* currentword  = cstr_new_cstring("");

	for (int index = 0; index < words->length; index++)
	{
	    char* word = words->data[index];

	    if (strcmp(word, "is") == 0) continue;

	    if (map_get(sl.fields, word) != NULL)
	    {
		if (currentword != NULL && currentfield != NULL) MPUT(sl.filtermap, currentfield, currentword);

		if (currentword) REL(currentword);
		if (currentfield) REL(currentfield);

		// store as field
		currentfield = word;
		currentword  = cstr_new_cstring("");
	    }
	    else
	    {
		currentword = cstr_append(currentword, word);
		currentword = cstr_append(currentword, " ");
	    }
	}

	// add final words

	if (currentword != NULL && currentfield != NULL) MPUT(sl.filtermap, currentfield, currentword);
	else if (currentword != NULL) MPUT(sl.filtermap, "*", currentword);

	if (currentword) REL(currentword);
	if (currentfield) REL(currentfield);

	printf("FILTER:\n");
	mem_describe(sl.filtermap, 0);
    }
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
	vec_sort(sl.visible_songs, songlist_comp_entry);
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

void songlist_set_fields(map_t* fields)
{
    if (sl.fields) REL(sl.fields);
    sl.fields = NULL;
    if (fields) sl.fields = RET(fields);
}

vec_t* songlist_get_visible_songs()
{
    return sl.visible_songs;
}

#endif
