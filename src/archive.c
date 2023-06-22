#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct archive_entry {
    int offset;
    int size;
    char name[24];
};

struct archive {
    FILE* f;
    int n_entries;
    struct archive_entry* entries;
};

Archive Archive_open(const char* path) {
    Archive self = malloc(sizeof(struct archive));
    self->f = fopen(path, "rb");
    if (!self->f) return NULL;
    struct {
        char magic[4];
        int version;
        int n_entries;
        int pad;
    } hdr;
    fread(&hdr, sizeof(hdr), 1, self->f);
    self->n_entries = hdr.n_entries;
    self->entries = malloc(sizeof(struct archive_entry) * self->n_entries);
    fread(self->entries, sizeof(struct archive_entry), self->n_entries, self->f);
    return self;
}

void Archive_close(Archive self) {
    fclose(self->f);
    free(self->entries);
    free(self);
}

char* archive_read_text(Archive self, const char* name) {
    for (int i = 0; i < self->n_entries; i++) {
        if (strncmp(self->entries[i].name, name, 24) != 0) continue;
        char* buf = malloc(self->entries[i].size + 1);
        fseek(self->f, self->entries[i].offset, SEEK_SET);
        fread(buf, self->entries[i].size, 1, self->f);
        buf[self->entries[i].size] = 0;
        return buf;
    }
    return NULL; // todo: assert
}

size_t Archive_getSize(Archive self, const char* name) {
    for (int i = 0; i < self->n_entries; i++) {
        if (strncmp(self->entries[i].name, name, 24) != 0) continue;
        return self->entries[i].size;
    }
    return 0;
}

size_t Archive_read(Archive self, const char* name, size_t size, void* dst) {
    for (int i = 0; i < self->n_entries; i++) {
        if (strncmp(self->entries[i].name, name, 24) != 0) continue;
        fseek(self->f, self->entries[i].offset, SEEK_SET);
        return fread(dst, self->entries[i].size, 1, self->f);
    }
    Log_warn("file not found in archive: '%s'", name);
    return 0;
}
