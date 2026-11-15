#ifndef MUSICDB_H
#define MUSICDB_H

#include <stddef.h>
#include <stdio.h>

typedef struct Artist Artist;
typedef struct MusicDB MusicDB;

/* For L=1 Layout Card (I am assuming it means UID and not USGID)
* My UID is 120972485
* For the member order, I want short year, char *title, unsigned int id, const Artist *artist, float rating, unsigned long num_rated
*/
struct Song {
    short year;
    char *title;
    unsigned int id;
    const Artist *artist;
    float rating;
    unsigned long num_rated;
};
typedef struct Song Song;

struct Artist {
    char *name;
    size_t n_songs;
    size_t cap;
    Song **songs;
};

typedef int (*song_pred)(const Song *s, const void *key);

MusicDB *db_create(void);
void db_destroy(MusicDB *db);
int db_load(MusicDB *db, const char *dir);
size_t db_song_count(const MusicDB *db);
size_t db_artist_count(const MusicDB *db);
const Song *db_get(const MusicDB *db, size_t i);
int db_push(MusicDB *db, const char *title, const char *artist,
            double rating, int year, unsigned long num_rated);
void db_disp(const MusicDB *db, FILE *out, size_t lim);
size_t db_search(const MusicDB *db, song_pred pred, const void *key,
                 const Song ***out);
void db_sort(MusicDB *db, int card);

// Change this to pointer only, doesn’t meet reqs rn
// Fixed
size_t db_count_decade(const MusicDB *db, int start, const Song **best);
//This also, I forgot to use pointers
// Fixed

const Artist *db_top_artist(const MusicDB *db);

#endif

