
/* 
 * When writing code, I sometimes used my own native language (turkish) for var names for ease of use (also stress under time pressure), so you can find the definitions of variable names below:
 * bul_artist  = find_artist
 * ekle_artist = add_artist
 * yol         = path (for the file path buffer)
 * sayi        = number
 * sayac       = counter
 * en_iyi      = best (for best song)
 * max_oy      = max_raters or max_votes
 * son         = end (to make pointer boundary)
 */
// I didn't want to mess up the code by trying to arrange var names manually afterwards (otherwise if I mess up from stress it will be very hard to fix during other debugging), so I included definitions instead
#define _POSIX_C_SOURCE 200809L
#include "musicdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

struct MusicDB {
    Song **songs;
    size_t n_songs;
    size_t cap_songs;

    Artist **artists;
    size_t n_artists;
    size_t cap_artists;
};

static void trim_whitespace(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || isspace((unsigned char)str[len - 1]))) {
        str[len - 1] = '\0';
        len--;
    }
}

MusicDB *db_create(void) {
    MusicDB *db = malloc(sizeof(MusicDB));
    if (!db) return NULL;
    db->n_songs = 0;
    db->cap_songs = 16;
    db->songs = malloc(db->cap_songs * sizeof(Song *));

    db->n_artists = 0;
    db->cap_artists = 16;
    db->artists = malloc(db->cap_artists * sizeof(Artist *));

    if (!db->songs || !db->artists) {
        free(db->songs);
        free(db->artists);
        free(db);
        return NULL;
    }
    return db;
}

void db_destroy(MusicDB *db) {
    if (!db) return;
    for (size_t i = 0; i < db->n_songs; i++) {
        free(db->songs[i]->title);
        free(db->songs[i]);
    }
    free(db->songs);

    for (size_t i = 0; i < db->n_artists; i++) {
        free(db->artists[i]->name);
        free(db->artists[i]->songs);
        free(db->artists[i]);
    }
    free(db->artists);
    free(db);
}

size_t db_song_count(const MusicDB *db) { return db ? db->n_songs : 0; }
size_t db_artist_count(const MusicDB *db) { return db ? db->n_artists : 0; }

const Song *db_get(const MusicDB *db, size_t i) {
    if (!db || i >= db->n_songs) return NULL;
    return db->songs[i];
}

static Artist *bul_artist(MusicDB *db, const char *name) {
    for (size_t i = 0; i < db->n_artists; i++) {
        if (strcmp(db->artists[i]->name, name) == 0) return db->artists[i];
    }
    return NULL;
}

static Artist *ekle_artist(MusicDB *db, const char *name) {
    Artist *existing = bul_artist(db, name);
    if (existing) return existing;

    if (db->n_artists >= db->cap_artists) {
        size_t new_cap = db->cap_artists * 2;
        Artist **temp = realloc(db->artists, new_cap * sizeof(Artist *));
        if (!temp) return NULL;
        db->artists = temp;
        db->cap_artists = new_cap;
    }
    Artist *a = malloc(sizeof(Artist));
    if (!a) return NULL;
    a->name = malloc(strlen(name) + 1);
    if (!a->name) { free(a); return NULL; }
    strcpy(a->name, name);
    a->n_songs = 0;
    a->cap = 4;
    a->songs = malloc(a->cap * sizeof(Song *));
    if (!a->songs) { free(a->name); free(a); return NULL; }
    db->artists[db->n_artists++] = a;
    return a;
}

int db_push(MusicDB *db, const char *title, const char *artist_name,
            double rating, int year, unsigned long num_rated) {
    if (!db || !title || !artist_name) return -1;
    if (year < 1000 || year > 3000) return -1;
    if (rating < 0.0 || rating > 5.0) return -1;
    
    unsigned long max_raters = _Generic(((Song *)0)->num_rated,
        unsigned char: (unsigned long)UCHAR_MAX,
        unsigned short: (unsigned long)USHRT_MAX,
        unsigned int: (unsigned long)UINT_MAX,
        unsigned long: (unsigned long)ULONG_MAX,
        default: (unsigned long)UINT_MAX
    );
    if (num_rated > max_raters) return -1;

    if (db->n_songs >= db->cap_songs) {
        size_t new_cap = db->cap_songs * 2;
        Song **temp = realloc(db->songs, new_cap * sizeof(Song *));
        if (!temp) return -1;
        db->songs = temp;
        db->cap_songs = new_cap;
    }

    Song *s = malloc(sizeof(Song));
    if (!s) return -1;
    s->title = malloc(strlen(title) + 1);
    if (!s->title) { free(s); return -1; }
    strcpy(s->title, title);

    Artist *art = ekle_artist(db, artist_name);
    if (!art) {
        free(s->title);
        free(s);
        return -1;
    }

    if (art->n_songs >= art->cap) {
        size_t new_cap = art->cap * 2;
        Song **temp = realloc(art->songs, new_cap * sizeof(Song *));
        if (!temp) {
            free(s->title);
            free(s);
            return -1;
        }
        art->songs = temp;
        art->cap = new_cap;
    }

    s->artist = art;
    s->rating = rating;
    s->year = (short)year;
    s->num_rated = num_rated;
    s->id = (unsigned int)db->n_songs;

    db->songs[db->n_songs++] = s;
    art->songs[art->n_songs++] = s;
    return 0;
}

int db_load(MusicDB *db, const char *dir) {
    char yol[512];
    FILE *fs, *fa, *fy, *fr, *fp;
    snprintf(yol, sizeof(yol), "%s/songs.txt", dir); fs = fopen(yol, "r");
    snprintf(yol, sizeof(yol), "%s/artists.txt", dir); fa = fopen(yol, "r");
    snprintf(yol, sizeof(yol), "%s/year.txt", dir); fy = fopen(yol, "r");
    snprintf(yol, sizeof(yol), "%s/ratings.txt", dir); fr = fopen(yol, "r");
    snprintf(yol, sizeof(yol), "%s/people_rated.txt", dir); fp = fopen(yol, "r");

    if (!fs || !fa || !fy || !fr || !fp) {
        if (fs) fclose(fs);
        if (fa) fclose(fa);
        if (fy) fclose(fy);
        if (fr) fclose(fr);
        if (fp) fclose(fp);
        return -1;
    }

    char *b1 = NULL, *b2 = NULL, *b3 = NULL, *b4 = NULL, *b5 = NULL;
    size_t s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0;
    int error = 0;

    while (1) {
        ssize_t r1 = getline(&b1, &s1, fs);
        ssize_t r2 = getline(&b2, &s2, fa);
        ssize_t r3 = getline(&b3, &s3, fy);
        ssize_t r4 = getline(&b4, &s4, fr);
        ssize_t r5 = getline(&b5, &s5, fp);

        int eof1 = feof(fs), eof2 = feof(fa), eof3 = feof(fy), eof4 = feof(fr), eof5 = feof(fp);
        int err1 = ferror(fs), err2 = ferror(fa), err3 = ferror(fy), err4 = ferror(fr), err5 = ferror(fp);

        if (err1 || err2 || err3 || err4 || err5) {
            error = 1;
            break;
        }

        int all_eof = (r1 == -1 && eof1) && (r2 == -1 && eof2) && (r3 == -1 && eof3) && (r4 == -1 && eof4) && (r5 == -1 && eof5);
        if (all_eof) break;

        if (r1 == -1 || r2 == -1 || r3 == -1 || r4 == -1 || r5 == -1) {
            error = 1;
            break;
        }

        trim_whitespace(b1);
        trim_whitespace(b2);
        trim_whitespace(b3);
        trim_whitespace(b4);
        trim_whitespace(b5);

        if (strlen(b1) == 0 || strlen(b2) == 0) { error = 1; break; }

        char *endptr;
        errno = 0;
        long year = strtol(b3, &endptr, 10);
        if (errno != 0 || endptr == b3 || *endptr != '\0' || year < 1000 || year > 3000) { error = 1; break; }

        if (strlen(b4) == 0) { error = 1; break; }
        errno = 0;
        double rating = strtod(b4, &endptr);
        if (errno != 0 || endptr == b4 || *endptr != '\0' || !isfinite(rating) || rating < 0.0 || rating > 5.0) { error = 1; break; }

        if (strlen(b5) == 0 || strchr(b5, '-') != NULL) { error = 1; break; }
        errno = 0;
        unsigned long raters = strtoul(b5, &endptr, 10);
        if (errno != 0 || endptr == b5 || *endptr != '\0') { error = 1; break; }

        if (db_push(db, b1, b2, rating, (int)year, raters) != 0) { error = 1; break; }
    }

    free(b1); free(b2); free(b3); free(b4); free(b5);
    fclose(fs); fclose(fa); fclose(fy); fclose(fr); fclose(fp);

    return error ? -1 : 0;
}

void db_disp(const MusicDB *db, FILE *out, size_t lim) {
    if (!db || !out) return;
    fprintf(out, "%-35s %-25s %-5s %-8s %s\n", "Song Title", "Artist", "Year", "Rating", "People Rated");
    // sayi = number
    size_t sayi = (lim == 0 || lim > db->n_songs) ? db->n_songs : lim;
    for (size_t i = 0; i < sayi; i++) {
        const Song *s = db->songs[i];
        fprintf(out, "%-35.35s %-25.25s %-5d %-8.2f %lu\n",
                s->title, s->artist->name, (int)s->year, (double)s->rating, (unsigned long)s->num_rated);
    }
}

size_t db_search(const MusicDB *db, song_pred pred, const void *key, const Song ***out) {
    if (!db || !pred) { if (out) *out = NULL; return 0; }
    if (db->n_songs == 0) { if (out) *out = NULL; return 0; }

    const Song **res = malloc(db->n_songs * sizeof(const Song *));
    if (!res) { if (out) *out = NULL; return 0; }

    size_t count = 0;
    for (size_t i = 0; i < db->n_songs; i++) {
        if (pred(db->songs[i], key)) {
            res[count++] = db->songs[i];
        }
    }

    if (count == 0) {
        free(res);
        if (out) *out = NULL;
        return 0;
    }

    const Song **temp = realloc(res, count * sizeof(const Song *));
    if (temp) res = temp;

    if (out) *out = res;
    return count;
}

static int cmp_card0(const void *a, const void *b) {
    const Song *s1 = *(const Song **)a;
    const Song *s2 = *(const Song **)b;
    if (s1->rating != s2->rating) return (s2->rating > s1->rating) ? 1 : -1;
    if (s1->num_rated != s2->num_rated) return (s2->num_rated > s1->num_rated) ? 1 : -1;
    return (s1->id > s2->id) ? 1 : ((s1->id < s2->id) ? -1 : 0);
}

static int cmp_card1(const void *a, const void *b) {
    const Song *s1 = *(const Song **)a;
    const Song *s2 = *(const Song **)b;
    if (s1->year != s2->year) return (s1->year > s2->year) ? 1 : -1;
    int t = strcmp(s1->title, s2->title);
    if (t != 0) return t;
    return (s1->id > s2->id) ? 1 : ((s1->id < s2->id) ? -1 : 0);
}

static int cmp_card2(const void *a, const void *b) {
    const Song *s1 = *(const Song **)a;
    const Song *s2 = *(const Song **)b;
    int t = strcmp(s1->artist->name, s2->artist->name);
    if (t != 0) return t;
    if (s1->year != s2->year) return (s2->year > s1->year) ? 1 : -1;
    return (s1->id > s2->id) ? 1 : ((s1->id < s2->id) ? -1 : 0);
}

static int cmp_card3(const void *a, const void *b) {
    const Song *s1 = *(const Song **)a;
    const Song *s2 = *(const Song **)b;
    if (s1->num_rated != s2->num_rated) return (s2->num_rated > s1->num_rated) ? 1 : -1;
    if (s1->rating != s2->rating) return (s2->rating > s1->rating) ? 1 : -1;
    return (s1->id > s2->id) ? 1 : ((s1->id < s2->id) ? -1 : 0);
}

void db_sort(MusicDB *db, int card) {
    if (!db || db->n_songs == 0) return;
    if (card == 0) qsort(db->songs, db->n_songs, sizeof(Song *), cmp_card0);
    else if (card == 1) qsort(db->songs, db->n_songs, sizeof(Song *), cmp_card1);
    else if (card == 2) qsort(db->songs, db->n_songs, sizeof(Song *), cmp_card2);
    else if (card == 3) qsort(db->songs, db->n_songs, sizeof(Song *), cmp_card3);
}

/* PTR-ONLY */
size_t db_count_decade(const MusicDB *db, int start, const Song **best) {
    if (!db) { if (best) *best = NULL; return 0; }
    size_t sayac = 0;
    const Song *en_iyi = NULL;
    unsigned long max_oy = 0;
    Song **ptr = db->songs;
    Song **son = db->songs + db->n_songs;
    while (ptr < son) {
        const Song *s = *ptr;
        if (s->year >= start && s->year <= start + 9) {
            sayac++;
            if (!en_iyi || s->num_rated > max_oy || (s->num_rated == max_oy && s->id < en_iyi->id)) {
                max_oy = s->num_rated;
                en_iyi = s;
            }
        }
        ptr++;
    }
    if (best) *best = en_iyi;
    return sayac;
}

/* PTR-ONLY */
const Artist *db_top_artist(const MusicDB *db) {
    if (!db || db->n_artists == 0) return NULL;
    const Artist *top = NULL;
    Artist **ptr = db->artists;
    Artist **son = db->artists + db->n_artists;
    while (ptr < son) {
        const Artist *a = *ptr;
        if (!top || a->n_songs > top->n_songs || (a->n_songs == top->n_songs && strcmp(a->name, top->name) < 0)) {
            top = a;
        }
        ptr++;
    }
    return top;
}
