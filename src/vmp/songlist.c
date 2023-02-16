#ifndef songlist_h
#define songlist_h

#include "mt_map.c"
#include "mt_vector.c"

void         songlist_destroy();
mt_map_t*    songlist_get_song(int shuffle);
mt_map_t*    songlist_get_prev_song(mt_map_t* song);
mt_map_t*    songlist_get_next_song(mt_map_t* song);
void         songlist_set_songs(mt_vector_t* songs);
size_t       songlist_get_index(mt_map_t* song);
void         songlist_apply_filter();
void         songlist_apply_sorting();
void         songlist_set_filter(char* filter);
void         songlist_set_sorting(char* sorting);
void         songlist_set_fields(mt_map_t* fields);
void         songlist_set_numeric_fields(mt_map_t* fields);
char*        songlist_get_sorting();
mt_vector_t* songlist_get_visible_songs();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_string.c"
#include "mt_string_ext.c"
#include "mt_time.c"
#include "utf8.h"
#include <stdlib.h>

struct _songlist_t
{
    mt_map_t* fields;

    mt_vector_t* songs;         // all items in library
    mt_vector_t* visible_songs; // filtered items

    char*        filter;    // artist is Metallica album is Ride the lightning genre is Rock genre is Metal
    mt_map_t*    filtermap; // key - field,
    char*        sorting;   // 1 is ascending 0 is descending
    mt_vector_t* sortvec;
    mt_map_t*    numfields; // numeric fields
} sl = {0};

void songlist_destroy()
{
    if (sl.fields) REL(sl.fields);
    if (sl.songs) REL(sl.songs);
    if (sl.visible_songs) REL(sl.visible_songs);
    if (sl.filter) REL(sl.filter);
    if (sl.filtermap) REL(sl.filtermap);
    if (sl.sorting) REL(sl.sorting);
    if (sl.sortvec) REL(sl.sortvec);
    if (sl.numfields) REL(sl.numfields);

    sl.fields        = NULL;
    sl.songs         = NULL;
    sl.visible_songs = NULL;
    sl.filter        = NULL;
    sl.filtermap     = NULL;
    sl.sorting       = NULL;
    sl.sortvec       = NULL;
    sl.numfields     = NULL;
}

mt_map_t* songlist_get_song(int shuffle)
{
    mt_map_t* result = NULL;
    if (sl.visible_songs && sl.visible_songs->length > 0)
    {
	int index = 0;
	if (shuffle) index = rand() % sl.visible_songs->length;
	result = sl.visible_songs->data[index];
    }
    return result;
}

mt_map_t* songlist_get_prev_song(mt_map_t* song)
{
    mt_map_t* result = NULL;
    if (sl.visible_songs && sl.visible_songs->length > 0)
    {
	size_t index = mt_vector_index_of_data(sl.visible_songs, song);
	if (index < SIZE_MAX && index > 0)
	{
	    index -= 1;
	    result = sl.visible_songs->data[index];
	}
    }
    return result;
}

mt_map_t* songlist_get_next_song(mt_map_t* song)
{
    mt_map_t* result = NULL;
    if (sl.visible_songs && sl.visible_songs->length > 0)
    {
	size_t index = mt_vector_index_of_data(sl.visible_songs, song);
	if (index < SIZE_MAX && index < sl.visible_songs->length)
	{
	    index += 1;
	    result = sl.visible_songs->data[index];
	}
    }
    return result;
}

size_t songlist_get_index(mt_map_t* song)
{
    size_t result = mt_vector_index_of_data(sl.visible_songs, song);
    return result;
}

void songlist_apply_filter()
{
    if (sl.visible_songs)
	mt_vector_reset(sl.visible_songs);
    else
	sl.visible_songs = VNEW();

    mt_time(NULL);

    if (sl.filter != NULL)
    {
	mt_vector_t* filtered = VNEW();
	mt_vector_t* fields   = VNEW();

	mt_map_keys(sl.fields, fields);

	for (size_t index = 0;
	     index < sl.songs->length;
	     index++)
	{
	    mt_map_t* entry = sl.songs->data[index];

	    for (size_t fi = 0; fi < fields->length; fi++)
	    {
		char* field       = fields->data[fi];
		char* filtervalue = MGET(sl.filtermap, field);
		if (filtervalue == NULL)
		    filtervalue = MGET(sl.filtermap, "*");

		if (filtervalue != NULL)
		{
		    char* value = MGET(entry, field);

		    if (value != NULL)
		    {
			if (utf8casestr(value, filtervalue))
			{
			    VADD(filtered, entry);
			    break;
			}
		    }
		}
	    }
	}

	mt_vector_add_in_vector(sl.visible_songs, filtered);
	REL(filtered);
	REL(fields);
    }
    else
    {
	mt_vector_add_in_vector(sl.visible_songs, sl.songs);
    }
    mt_time("filtering");
}

void songlist_set_filter(char* filter)
{
    if (sl.filter != NULL)
	REL(sl.filter);
    sl.filter = NULL;
    if (filter)
	sl.filter = STRNC(filter);

    if (sl.filter)
    {
	mt_vector_t* words = mt_string_split(sl.filter, " "); // REL WORDS

	if (sl.filtermap)
	    REL(sl.filtermap);
	sl.filtermap = MNEW();

	char* currentfield = NULL;
	char* currentword  = STRNC("");

	for (size_t index = 0; index < words->length; index++)
	{
	    char* word = words->data[index];

	    if (strcmp(word, "is") == 0)
		continue;

	    if (mt_map_get(sl.fields, word) != NULL)
	    {
		if (currentword != NULL && currentfield != NULL)
		    MPUT(sl.filtermap, currentfield, currentword);

		if (currentword)
		    REL(currentword);
		if (currentfield)
		    REL(currentfield);

		// store as field
		currentfield = RET(word);
		currentword  = STRNC("");
	    }
	    else
	    {
		if (strlen(currentword) > 0)
		    currentword = mt_string_append(currentword, " ");
		currentword = mt_string_append(currentword, word);
	    }
	}

	// add final words

	if (currentword != NULL && currentfield != NULL)
	    MPUT(sl.filtermap, currentfield, currentword);
	else if (currentword != NULL)
	    MPUT(sl.filtermap, "*", currentword);

	if (currentword)
	    REL(currentword);
	if (currentfield)
	    REL(currentfield);

	REL(words);
    }

    if (sl.songs)
	songlist_apply_filter();
}

int songlist_comp_entry(void* left, void* right)
{
    mt_map_t* l = left;
    mt_map_t* r = right;

    for (size_t index = 0; index < sl.sortvec->length; index += 2)
    {
	char* field = sl.sortvec->data[index];
	char* sorts = sl.sortvec->data[index + 1];

	int dir = -1;
	if (sorts[0] == '1')
	    dir = 1;

	char* la = MGET(l, field);
	char* ra = MGET(r, field);

	if (MGET(sl.numfields, field))
	{
	    int ln = la ? atoi(la) : 0;
	    int rn = ra ? atoi(ra) : 0;

	    if (ln == rn)
		continue;

	    if (ln < rn)
		return dir * -1;
	    else
		return dir;
	}
	else
	{

	    if (la == NULL)
		la = "";
	    if (ra == NULL)
		ra = "";

	    if (strcmp(la, ra) == 0)
		continue;

	    return dir * strcmp(la, ra);
	}
    }

    return 0;
}

void songlist_apply_sorting()
{
    if (sl.sorting)
    {
	mt_time(NULL);
	mt_vector_sort(sl.songs, songlist_comp_entry);
	mt_time("sorting");
    }
}

void songlist_set_sorting(char* sorting)
{
    if (sl.sorting != NULL) REL(sl.sorting);
    sl.sorting = NULL;
    if (sorting)
    {
	sl.sorting = STRNC(sorting);
	if (sl.sortvec) REL(sl.sortvec);
	sl.sortvec = mt_string_split(sl.sorting, " ");
    }
    if (sl.songs) songlist_apply_sorting();
    if (sl.songs) songlist_apply_filter();
}

void songlist_set_songs(mt_vector_t* songs)
{
    if (sl.visible_songs) mt_vector_reset(sl.visible_songs);
    else sl.visible_songs = VNEW();

    if (sl.songs) REL(sl.songs);
    sl.songs = NULL;
    if (songs) sl.songs = RET(songs);

    if (songs->length > 0)
    {
	songlist_apply_sorting();
	songlist_apply_filter();
    }
}

void songlist_set_fields(mt_map_t* fields)
{
    if (sl.fields) REL(sl.fields);
    sl.fields = NULL;
    if (fields) sl.fields = RET(fields);
}

void songlist_set_numeric_fields(mt_map_t* fields)
{
    if (sl.numfields) REL(sl.numfields);
    sl.numfields = NULL;
    if (fields) sl.numfields = RET(fields);
}

mt_vector_t* songlist_get_visible_songs()
{
    return sl.visible_songs;
}

char* songlist_get_sorting()
{
    return sl.sorting;
}

#endif
