
#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int valid;
    int tag;
    unsigned long lru;
} CacheLine;

int s, E, b;
int S;
CacheLine **cache;
int hit_count = 0, miss_count = 0, eviction_count = 0;
unsigned long lru_counter = 0;

void init_cache() {
    S = 1 << s;
    cache = malloc(S * sizeof(CacheLine *));
    for (int i = 0; i < S; i++) {
        cache[i] = malloc(E * sizeof(CacheLine));
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru = 0;
        }
    }
}

void access_cache(unsigned long addr) {
    unsigned long tag = addr >> (s + b);
    unsigned long set = (addr >> b) & ((1UL << s) - 1);

    for (int i = 0; i < E; i++) {
        if (cache[set][i].valid && cache[set][i].tag == tag) {
            hit_count++;
            cache[set][i].lru = lru_counter++;
            return;
        }
    }

    miss_count++;

    for (int i = 0; i < E; i++) {
        if (!cache[set][i].valid) {
            cache[set][i].valid = 1;
            cache[set][i].tag = tag;
            cache[set][i].lru = lru_counter++;
            return;
        }
    }

    int lru_idx = 0;
    unsigned long min_lru = cache[set][0].lru;
    for (int i = 1; i < E; i++) {
        if (cache[set][i].lru < min_lru) {
            min_lru = cache[set][i].lru;
            lru_idx = i;
        }
    }
    eviction_count++;
    cache[set][lru_idx].tag = tag;
    cache[set][lru_idx].lru = lru_counter++;
}

void simulate_trace(const char *filename, int verbose) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    char op;
    unsigned long addr;
    int size;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, " %c %lx,%d", &op, &addr, &size) != 3)
            continue;

        if (op == 'I') continue;

        if (verbose) printf("%c %lx,%d", op, addr, size);

        if (op == 'L' || op == 'S') {
            access_cache(addr);
            if (verbose) printf(" %s", "hit/miss/eviction");
        } else if (op == 'M') {
            access_cache(addr);   // load
            access_cache(addr);   // store
            if (verbose) printf(" hit hit");
        }
        if (verbose) printf("\n");
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    int opt, verbose = 0;
    char *tracefile = NULL;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 's': s = atoi(optarg); break;
        case 'E': E = atoi(optarg); break;
        case 'b': b = atoi(optarg); break;
        case 't': tracefile = optarg; break;
        case 'v': verbose = 1; break;
        case 'h':
        default:
            printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
            exit(1);
        }
    }

    if (s <= 0 || E <= 0 || b <= 0 || !tracefile) {
        printf("Missing required arguments.\n");
        return 1;
    }

    init_cache();
    simulate_trace(tracefile, verbose);
    printSummary(hit_count, miss_count, eviction_count);

    for (int i = 0; i < S; i++) free(cache[i]);
    free(cache);

    return 0;
}
