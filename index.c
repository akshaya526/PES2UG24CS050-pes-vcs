#include "index.h"
#include "pes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// forward declaration
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ─── LOAD ─────────────────────────

int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(".pes/index", "r");
    if (!f) return 0;

    char hash_hex[65];

    while (fscanf(f, "%u %64s %lu %u %s",
                  &index->entries[index->count].mode,
                  hash_hex,
                  &index->entries[index->count].mtime_sec,
                  &index->entries[index->count].size,
                  index->entries[index->count].path) == 5) {

        hex_to_hash(hash_hex, &index->entries[index->count].hash);
        index->count++;
    }

    fclose(f);
    return 0;
}

// ─── SAVE ─────────────────────────

int index_save(const Index *index) {
    FILE *f = fopen(".pes/index", "w");
    if (!f) return -1;

    char hex[65];

    for (int i = 0; i < index->count; i++) {
        hash_to_hex(&index->entries[i].hash, hex);

        fprintf(f, "%u %s %lu %u %s\n",
                index->entries[i].mode,
                hex,
                index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }

    fclose(f);
    return 0;
}

// ─── ADD (SUPER SAFE VERSION) ─────────────────────────

int index_add(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("error opening file\n");
        return -1;
    }

    // get size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return -1;
    }

    char *data = malloc(size);
    if (!data) {
        fclose(f);
        return -1;
    }

    fread(data, 1, size, f);
    fclose(f);

    ObjectID hash;
    object_write(OBJ_BLOB, data, size, &hash);

    free(data);

    struct stat st;
    stat(path, &st);

    // add entry
    IndexEntry *e = &index->entries[index->count++];

    e->hash = hash;
    e->mode = 100644;
    e->mtime_sec = st.st_mtime;
    e->size = st.st_size;

    strcpy(e->path, path);

    return index_save(index);
}

// ─── STATUS ─────────────────────────

int index_status(const Index *index) {
    printf("Staged changes:\n");

    if (index->count == 0)
        printf("  (nothing to show)\n");
    else {
        for (int i = 0; i < index->count; i++)
            printf("  staged:     %s\n", index->entries[i].path);
    }

    printf("\n");
    return 0;
}