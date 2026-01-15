# Solution Process

## Basis

I self-study the CSAPP labs, so I don't need to create the handin tarball. I comment out the following line in the `Makefile`:

```txt
 # -tar -cvf ${USER}-handin.tar  csim.c trans.c
```

## Process

### Part A

We need to complete the `csim.c` file so that it takes the same command line arguments and produces the identical output as the reference simulator.

Firstly, we need to process the input arguments correctly. We use `getopt`, a library function for handling input parameters, to parse the input options and initialize the values. Note that we haven't comprehensively addressed all scenarios involving input parameters because this lab focuses on caching. Therefore, we don't concern ourselves with the security of the program; we only handle basic logic errors and focus most of our efforts on other logics.

The code is shown below:

```c
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
```

Here is the `printUsage` function:

```c
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
```

Then we need to define the cache structure. Since we don't actually need to retrieve or write data to the cache, we only need to have a valid flag and a tag. Also, to apply the LRU algorithm, we need to introduce a `time_stamp`. Finally, we need to manually allocate and deallocate the memory.

We define the cache structure as following:

```c
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
```

And we can allocate the memory and initialize the whole cache by this:

```c
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
```

Next, we handle the caching logic. We define several global parameters to record the number of times a hit, a miss, or a eviction occurred. We also need to define a global timestamp variable to facilitate subsequent LRU algorithm processing:

```c
// Define global parameters counting the performance
int hits = 0;
int misses = 0;
int evictions = 0;
// Define a timestamp to record the point
int tt = 0;
```

For the cache handling code, we first need to iterate through the cache set that needs to be checked. If a cache hit occurs, we can directly update variables and return. If a cache hit does not occur, we need to fill it into an empty cache line or into a line that needs to be evicted, and record the corresponding changes:

```c
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
```

Finally, we process each line from the file, and selectively output certain content based on whether or not it requires output verbosely. We should note that we run this lab on a 64-bit x86-64 system, so the address consists of 64 bits, and we should use `unsigned long` to convert from the hex value:

```c
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
```

To sum up, all of the code is shown below:

```c
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
```

We type `make && ./test-csim` to check the result:

```txt
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```

Mission accomplished!

### Part B

As the reference says, we can optimize our code specifically for the following three cases, with using the reference simulator to replay this trace on a cache with parameters (s = 5, E = 1, b = 5):

- 32 \* 32: requires the number of misses < 300
- 64 \* 64: requires the number of misses < 1300
- 61 \* 67: requires the number of misses < 2000

This means we have a direct cache with a block size of 32 bytes, and with a set size of 32 bytes as well, we can know the size of the cache is 1024 bytes, namely 1KB.

#### 32\*32

The example transpose function is a simple baseline one. It has good spatial locality for `A`, while bad for `B`. We use `make && ./test-trans -M 32 -N 32` to generate test results:

```txt
func 1 (Simple row-wise scan transpose): hits:869, misses:1184, evictions:1152
```

Evidently, it didn't satisfy the requirement for `32*32` arrays.

We notice the two lines in `tracegen.c` file:

```c
static int A[256][256];
static int B[256][256];
```

This clarifies two things:

1. As global static variables, A and B have fixed addresses.
2. Since $256 * 256$ is divisible by 1024, the starting addresses of A and B are located in the same cache block.

We then use the `./csim-ref -v -s 5 -E 1 -b 5 -t trace.f1` to view the specific addresses of A and B:

```txt
...
L 10c0a0,4 miss eviction
S 14c0a0,4 miss eviction
L 10c0a4,4 miss eviction
S 14c120,4 miss
L 10c0a8,4 hit
S 14c1a0,4 miss
...
```

Based on the result above, we can conclude that A starts at address `10c0a0` and B starts at address `14c0a0`. The difference between them is exactly the size of $256 * 256$ integers in bytes.

However, there is a strange thing: the address of the next line of B is only 128 bytes longer than the previous line, which is 32 integers, instead of 256 integers.
The reason lies in the function `trans` in `trans.c`:

```c
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }
}
```

The address on the next line is calculated based on the size M passed in. This gives us the illusion that B is `M*N` in size, and the same applies to A.

Clearly, `miss` consists of three parts:

1. A read miss
2. B read miss
3. A cache conflict occurs between A and B

A exhibits good spatial locality; theoretically, it will only miss once out of every eight reads.

B is stored column-wise, and this cached row will be evicted by the subsequent 8th column store operation -- each row fills 4 cache lines, so theoretically 8 rows will occupy and evict all cache space. Therefore, for B, each store operation will result in a miss.

Whenever `i` equals `j` (i.e., on the diagonal), a cache conflict will occur between A and B, which will cause A to miss in its next read.

We can divide this $32*32$ array into 16 smaller $8*8$ arrays and process them separately. We demonstrate good spatial locality in reading data from A by reading eight numbers from a single line at once. Our store operation on B also exhibits good spatial locality because it only stores eight consecutive lines (as mentioned above, the cache can hold eight consecutive lines without eviction).

The code is as follows:

```c
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  int v1, v2, v3, v4, v5, v6, v7, v8;
  if (M == 32) {
    for (int i = 0; i < N; i += 8) {
      for (int j = 0; j < M; j += 8) {
        for (int k = 0; k < 8; ++k) {
          v1 = A[i + k][j];
          v2 = A[i + k][j + 1];
          v3 = A[i + k][j + 2];
          v4 = A[i + k][j + 3];
          v5 = A[i + k][j + 4];
          v6 = A[i + k][j + 5];
          v7 = A[i + k][j + 6];
          v8 = A[i + k][j + 7];
          B[j][i + k] = v1;
          B[j + 1][i + k] = v2;
          B[j + 2][i + k] = v3;
          B[j + 3][i + k] = v4;
          B[j + 4][i + k] = v5;
          B[j + 5][i + k] = v6;
          B[j + 6][i + k] = v7;
          B[j + 7][i + k] = v8;
        }
      }
    }
  }
}
```

The test results for this code are as follows:

```txt
func 0 (Transpose submission): hits:1765, misses:288, evictions:256
```

We have met the requirements, but this code still has a cache conflict issue. For example, read `A[0][0]~A[0][7]` and store them in `B[0][0]~B[7][0]`, but when it comes to read `A[1][0]~A[1][7]`, the cached `B[1][0]~B[1][7]` would be evicted.

We can firstly store `A[i+k][j+l]` to `B[j+k][i+l]`, and then interchange `B[j+k][i+l]` with `B[j+l][i+k]`. Finally we can get from `A[i][j]` to `B[j][i]`, the code is as follows:

```c
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  int v1, v2, v3, v4, v5, v6, v7, v8;
  if (M == 32) {
    for (int i = 0; i < N; i += 8) {
      for (int j = 0; j < M; j += 8) {
        for (int k = 0; k < 8; ++k) {
          v1 = A[i + k][j];
          v2 = A[i + k][j + 1];
          v3 = A[i + k][j + 2];
          v4 = A[i + k][j + 3];
          v5 = A[i + k][j + 4];
          v6 = A[i + k][j + 5];
          v7 = A[i + k][j + 6];
          v8 = A[i + k][j + 7];
          B[j + k][i] = v1;
          B[j + k][i + 1] = v2;
          B[j + k][i + 2] = v3;
          B[j + k][i + 3] = v4;
          B[j + k][i + 4] = v5;
          B[j + k][i + 5] = v6;
          B[j + k][i + 6] = v7;
          B[j + k][i + 7] = v8;
        }
        for (int k = 0; k < 8; ++k) {
          for (int l = 0; l < k; ++l) {
            v1 = B[j + k][i + l];
            B[j + k][i + l] = B[j + l][i + k];
            B[j + l][i + k] = v1;
          }
        }
      }
    }
  }
}
```

We operate on the entire line of B each time. We also filter out steps on the diagonal, thus completely eliminating cache conflicts.

The test results for this code are as follows:

```txt
func 0 (Transpose submission): hits:3585, misses:260, evictions:228
```

#### 64\*64

For a $64*64$ array, we can reuse the above method, except that the number of rows a cache can hold changes from 8 to 4. If we continue using the $8*8$ partitioning method, the following code will result in a large number of misses and evictions:

```c
for (int k = 0; k < 8; ++k) {
  for (int l = 0; l < k; ++l) {
    v1 = B[j + k][i + l];
    B[j + k][i + l] = B[j + l][i + k];
    B[j + l][i + k] = v1;
  }
}
```

The test results for code using the $8*8$ partitioning method are as follows:

```txt
func 0 (Transpose submission): hits:12033, misses:3332, evictions:3300
```

Therefore, we reduce the size of the partition to $4*4$:

```c
for (int i = 0; i < N; i += 4) {
  for (int j = 0; j < M; j += 4) {
    for (int k = 0; k < 4; ++k) {
      v1 = A[i + k][j];
      v2 = A[i + k][j + 1];
      v3 = A[i + k][j + 2];
      v4 = A[i + k][j + 3];
      B[j + k][i] = v1;
      B[j + k][i + 1] = v2;
      B[j + k][i + 2] = v3;
      B[j + k][i + 3] = v4;
    }
    for (int k = 0; k < 4; ++k) {
      for (int l = 0; l < k; ++l) {
        v1 = B[j + k][i + l];
        B[j + k][i + l] = B[j + l][i + k];
        B[j + l][i + k] = v1;
      }
    }
  }
}
```

The test results for this code are as follows:

```txt
func 0 (Transpose submission): hits:12737, misses:1604, evictions:1572
```

This isn't a perfect solution, but it's the most cost-effective. If you want a better solution, you can refer to [this one](https://github.com/zhuozhiyongde/Introduction-to-Computer-System-2023Fall-PKU/tree/main/05-Cache-Lab).

#### 61\*67

61 is not a multiple of 8. This means that the transpose B operation is less likely to miss or eviction.
Therefore, we first process it according to the $8*8$ division, and then process the remaining parts separately.

The code are as follows:

```c
for (int i = 0; i < 64; i += 8) {
  for (int j = 0; j < 56; j += 8) {
    for (int k = 0; k < 8; ++k) {
      v1 = A[i + k][j];
      v2 = A[i + k][j + 1];
      v3 = A[i + k][j + 2];
      v4 = A[i + k][j + 3];
      v5 = A[i + k][j + 4];
      v6 = A[i + k][j + 5];
      v7 = A[i + k][j + 6];
      v8 = A[i + k][j + 7];
      B[j + k][i] = v1;
      B[j + k][i + 1] = v2;
      B[j + k][i + 2] = v3;
      B[j + k][i + 3] = v4;
      B[j + k][i + 4] = v5;
      B[j + k][i + 5] = v6;
      B[j + k][i + 6] = v7;
      B[j + k][i + 7] = v8;
    }
    for (int k = 0; k < 8; ++k) {
      for (int l = 0; l < k; ++l) {
        v1 = B[j + k][i + l];
        B[j + k][i + l] = B[j + l][i + k];
        B[j + l][i + k] = v1;
      }
    }
  }
}
for (int i = 0; i < 65; i += 5) {
  for (int k = 0; k < 5; ++k) {
    v1 = A[i + k][56];
    v2 = A[i + k][57];
    v3 = A[i + k][58];
    v4 = A[i + k][59];
    v5 = A[i + k][60];
    B[56 + k][i] = v1;
    B[56 + k][i + 1] = v2;
    B[56 + k][i + 2] = v3;
    B[56 + k][i + 3] = v4;
    B[56 + k][i + 4] = v5;
  }
  for (int k = 0; k < 5; ++k) {
    for (int l = 0; l < k; ++l) {
      v1 = B[56 + k][i + l];
      B[56 + k][i + l] = B[56 + l][i + k];
      B[56 + l][i + k] = v1;
    }
  }
}
for (int i = 65; i < 67; ++i) {
  v1 = A[i][56];
  v2 = A[i][57];
  v3 = A[i][58];
  v4 = A[i][59];
  v5 = A[i][60];
  B[56][i] = v1;
  B[57][i] = v2;
  B[58][i] = v3;
  B[59][i] = v4;
  B[60][i] = v5;
}
for (int j = 0; j < 54; j += 3) {
  for (int k = 0; k < 3; ++k) {
    v1 = A[64 + k][j];
    v2 = A[64 + k][j + 1];
    v3 = A[64 + k][j + 2];
    B[j + k][64] = v1;
    B[j + k][65] = v2;
    B[j + k][66] = v3;
  }
  for (int k = 0; k < 3; ++k) {
    for (int l = 0; l < k; ++l) {
      v1 = B[j + k][64 + l];
      B[j + k][64 + l] = B[j + l][64 + k];
      B[j + l][64 + k] = v1;
    }
  }
}
for (int i = 64; i < 67; ++i) {
  v1 = A[i][54];
  v2 = A[i][55];
  B[54][i] = v1;
  B[55][i] = v2;
}
```

The test results for this code are as follows:

```txt
func 0 (Transpose submission): hits:13151, misses:2036, evictions:2004
```

We achieved a near-perfect score. The important thing is not the score, but the basic logic of optimizing cache hits, namely, block segmentation.
