#include "musicdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

struct q2_key {
    double min_rating;
    unsigned long min_rated;
};

static int q2_pred(const Song *s, const void *key) {
    const struct q2_key *k = (const struct q2_key *)key;
    
    // I need to use C11 standard Generic to make the threshold wanted type
    _Generic(((Song *)0)->rating,
        float: (void)((float)k->min_rating),
        double: (void)(k->min_rating),
        default: (void)(k->min_rating)
    );

    // generic selection for threshold -> wanted Song.rating type
    #define CONVERT_RATING(val) _Generic(((Song *)0)->rating, \
        float: (float)(val), \
        double: (double)(val), \
        default: (double)(val) \
    )

    return (s->rating >= CONVERT_RATING(k->min_rating) && s->num_rated >= k->min_rated);
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    const char *dir = argv[1];
    int id = atoi(argv[2]);
    int a = (id / 10) % 10;
    int b = id % 10;

    int L = (a + b) % 4;
    int D = a % 5;
    int T = b % 5;
    int S = (a + 2 * b) % 4;

    MusicDB *db = db_create();
    if (!db) return 1;
    if (db_load(db, dir) != 0) {
        db_destroy(db);
        return 1;
    }

    printf("ENEB340 REPORT\n");
    printf("card: L=%d D=%d T=%d S=%d\n", L, D, T, S);
    
    size_t max_end = 0;
    size_t off;
    
    off = offsetof(Song, year) + sizeof(((Song *)0)->year);
    if (off > max_end) max_end = off;
    off = offsetof(Song, title) + sizeof(((Song *)0)->title);
    if (off > max_end) max_end = off;
    off = offsetof(Song, id) + sizeof(((Song *)0)->id);
    if (off > max_end) max_end = off;
    off = offsetof(Song, artist) + sizeof(((Song *)0)->artist);
    if (off > max_end) max_end = off;
    off = offsetof(Song, rating) + sizeof(((Song *)0)->rating);
    if (off > max_end) max_end = off;
    off = offsetof(Song, num_rated) + sizeof(((Song *)0)->num_rated);
    if (off > max_end) max_end = off;

    size_t tail_padding = sizeof(Song) - max_end;
    printf("[A] sizeof(Song)=%zu tail_padding=%zu\n", sizeof(Song), tail_padding);
    printf("[A] offsets title=%zu artist=%zu year=%zu rating=%zu num_rated=%zu id=%zu\n",
           offsetof(Song, title), offsetof(Song, artist), offsetof(Song, year),
           offsetof(Song, rating), offsetof(Song, num_rated), offsetof(Song, id));

    printf("[B] songs=%zu artists=%zu\n", db_song_count(db), db_artist_count(db));
    
    const Song *s0 = db_get(db, 0);
    if (s0) {
        printf("[B] song[0]: %s | %s | %d | %.2f | %lu\n",
               s0->title, s0->artist->name, (int)s0->year, (double)s0->rating, (unsigned long)s0->num_rated);
    }
    
    const Artist *top_a = db_top_artist(db);
    if (top_a) {
        printf("[B] top_artist: %s (%zu songs)\n", top_a->name, top_a->n_songs);
    }

    char push_title1[64];
    char push_artist1[64];
    snprintf(push_title1, sizeof(push_title1), "Exam Song %04d", id);
    snprintf(push_artist1, sizeof(push_artist1), "Exam Artist %04d", id);

    int res1 = db_push(db, push_title1, push_artist1, 4.10, 1985, 1250000);
    if (res1 != 0) { db_destroy(db); return 1; }
    printf("[C] push1 -> songs=%zu artists=%zu\n", db_song_count(db), db_artist_count(db));

    char push_title2[64];
    snprintf(push_title2, sizeof(push_title2), "Exam Song %04d (Reprise)", id);
    int res2 = db_push(db, push_title2, "The Beatles", 3.70, 1985, 1250000);
    if (res2 != 0) { db_destroy(db); return 1; }
    printf("[C] push2 -> songs=%zu artists=%zu\n", db_song_count(db), db_artist_count(db));

    const Song *push2_song = db_get(db, db_song_count(db) - 1);
    size_t beatles_songs = push2_song ? push2_song->artist->n_songs : 0;
    printf("[C] beatles_songs=%zu\n", beatles_songs);

    int decade_start = 1960 + D * 10;
    const Song *best_dec = NULL;
    size_t dec_c = db_count_decade(db, decade_start, &best_dec);
    printf("[D] Q1 decade=%d count=%zu best_id=%u best_title=%s\n",
           decade_start, dec_c, best_dec ? best_dec->id : 0, best_dec ? best_dec->title : "none");

    // My 5 thresholds (0 to 4): 3.70, 3.90, 4.10, 4.30, 4.50
    double thresholds[5] = {3.70, 3.90, 4.10, 4.30, 4.50};
    double min_rat = thresholds[T];
    unsigned long min_raters = 1000000UL + 100000UL * T;
    struct q2_key k = {min_rat, min_raters};
    const Song **s_out = NULL;
    size_t m_c = db_search(db, q2_pred, &k, &s_out);
    unsigned int first_id = (m_c > 0 && s_out) ? s_out[0]->id : 0;
    unsigned int last_id = (m_c > 0 && s_out) ? s_out[m_c - 1]->id : 0;
    printf("[D] Q2 rating>=%.2f rated>=%lu count=%zu first_id=%u last_id=%u\n", (double)k.min_rating, (unsigned long)k.min_rated, m_c, first_id, last_id);
    free(s_out);

    db_sort(db, S);
    printf("[D] sort card %d top5 ids: ", S);
    for (int i = 0; i < 5 && (size_t)i < db_song_count(db); i++) {
        if (i > 0) printf(" ");
        printf("%u", db_get(db, i)->id);
    }
    printf("\n");

    db_disp(db, stdout, 3);
    db_destroy(db);
    return 0;
}
