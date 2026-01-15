#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

// Define global parameters counting the performance
int hits = 0;
int misses = 0;
int evictions = 0;
// Define a timestamp to record the point
int tt = 0;

// Define structure of cache line
struct cache_line {
  char valid;
  int tag;
  int time_stamp;
};

// Define cache line
typedef struct cache_line line;

// Define cache set
typedef line *set;

// Define cache cache
typedef set *cache;

// Load, store or modify the data with cache
// Return 0 if miss, 1 if hit, 2 if eviction happened
char useCache(cache cc, unsigned int index, int tag, int lines_per_set,
              char modify) {
  set st = cc[index];
  unsigned int line_index = 0;
  int line_time_stamp = st[0].time_stamp;

  for (int i = 0; i < lines_per_set; ++i) {
    line *tmp = &st[i];
    // hit
    if (tmp->valid && tmp->tag == tag) {
      if (modify) {
        ++hits;
      }
      ++hits;
      tmp->time_stamp = ++tt;
      return 1;
    }
    // record the least recent used block or empty block
    if (tmp->time_stamp < line_time_stamp) {
      line_index = i;
      line_time_stamp = tmp->time_stamp;
    }
  }

  // miss
  ++misses;
  if (modify) {
    ++hits;
  }
  // evict the least recent used block or fill in the empty block
  line *chosen = &st[line_index];
  chosen->tag = tag;
  chosen->time_stamp = ++tt;
  if (chosen->valid) {
    ++evictions;
    return 2;
  } else {
    chosen->valid = 1;
    return 0;
  }
}

// Initialize cache
cache getCache(int lines_per_set, int set_index_bits) {
  int length = 1 << set_index_bits;
  cache res = (cache)malloc(sizeof(set) * length);
  for (int i = 0; i < length; ++i) {
    res[i] = (set)malloc(sizeof(line) * lines_per_set);
    for (int j = 0; j < lines_per_set; ++j) {
      line *tmp = &res[i][j];
      tmp->valid = 0;
      tmp->tag = 0;
      tmp->time_stamp = 0;
    }
  }
  return res;
}

// Output help message
void printUsage() {
  puts("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>");
  puts("Options:");
  puts("  -h         Print this help message.");
  puts("  -v         Optional verbose flag.");
  puts("  -s <num>   Number of set index bits.");
  puts("  -E <num>   Number of lines per set.");
  puts("  -b <num>   Number of block offset bits.");
  puts("  -t <file>  Trace file.");
  puts("");
  puts("Examples:");
  puts(" linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace");
  puts(" linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace");
}

int main(int argc, char *argv[]) {
  // No input
  if (argc == 1) {
    printUsage();
    exit(EXIT_FAILURE);
  }

  // Define input arguments
  int v = 0;
  int s = -1;
  int e = -1;
  int b = -1;
  FILE *t = NULL;

  // Read arguments
  int opt;
  while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt) {
    case 'h':
      printUsage();
      exit(EXIT_SUCCESS);
    case 'v':
      v = 1;
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      e = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      t = fopen(optarg, "r");
      break;
    default:
      // Read undefined option
      printUsage();
      exit(EXIT_FAILURE);
    }
  }

  // Verify arguments
  if (s <= 0 || e <= 0 || b <= 0 || t == NULL) {
    printUsage();
    exit(EXIT_FAILURE);
  }

  cache c = getCache(e, s);
  // Read from file and process with cache
  char operation;
  unsigned long address;
  int size;
  unsigned int index;
  int tag;
  char res;
  char *str;
  while (fscanf(t, " %c %lx,%d\n", &operation, &address, &size) == 3) {
    index = (address >> b) & ((1 << s) - 1);
    tag = address >> (b + s);
    switch (operation) {
    case 'I':
      continue;
    case 'S':
    case 'L':
      res = useCache(c, index, tag, e, 0);
      if (v) {
        str = res == 1 ? "hit" : (res == 0 ? "miss" : "miss eviction");
        printf("%c %lx,%d %s\n", operation, address, size, str);
      }
      break;
    case 'M':
      res = useCache(c, index, tag, e, 1);
      if (v) {
        str = res == 1 ? "hit hit"
                       : (res == 0 ? "miss hit" : "miss eviction hit");
        printf("%c %lx,%d %s\n", operation, address, size, str);
      }
      break;
    }
  }

  // Remember to free up resource
  free(c);
  fclose(t);

  // Output the result
  printSummary(hits, misses, evictions);
  return 0;
}
