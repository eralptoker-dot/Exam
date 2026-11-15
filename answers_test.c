#include "musicdb.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

struct ReorderedSong {
    char *title;
    const Artist *artist;
    unsigned int id;
    unsigned int num_rated;
    float rating;
    short year;
};

int main(void)
{
    MusicDB *db = db_create();

    if (!db) {
        fprintf(stderr, "The database could not be formed.\n");
        return 1;
    }

    if (db_load(db, "data") != 0 || db_song_count(db) < 2) {
        fprintf(stderr, "Couldn't load at least two songs.\n");
        db_destroy(db);
        return 1;
    }

    const Song *first = db_get(db, 0);
    const Song *second = db_get(db, 1);

    printf("First Song: %p\n", (void *)first);
    printf("Second song:  %p\n", (void *)second);
    printf("1st Title:     %p\n", (void *)first->title);
    printf("2nd Title:     %p\n", (void *)second->title);

    uintptr_t a = (uintptr_t)first;
    uintptr_t b = (uintptr_t)second;
    uintptr_t difference = a > b ? a - b : b - a;

    printf("Absolute Address Difference: %" PRIuPTR " byte\n", difference);
    printf("sizeof(Song):       %zu byte\n\n", sizeof(Song));

    size_t end = 0;

#define SHOW_MEMBER(member) do {                                     \
    size_t offset = offsetof(Song, member);                          \
    size_t size = sizeof(((Song *)0)->member);                        \
    printf("%-10s -> offset=%zu, size=%zu, padding before=%zu\n", \
           #member, offset, size, offset - end);                     \
    end = offset + size;                                            \
} while (0)

    SHOW_MEMBER(year);
    SHOW_MEMBER(title);
    SHOW_MEMBER(id);
    SHOW_MEMBER(artist);
    SHOW_MEMBER(rating);
    SHOW_MEMBER(num_rated);

#undef SHOW_MEMBER

    printf("Tail Padding: %zu byte\n", sizeof(Song) - end);
    printf("Reordered Size:     %zu byte\n", sizeof(struct ReorderedSong));

    db_destroy(db);
    return 0;
}
