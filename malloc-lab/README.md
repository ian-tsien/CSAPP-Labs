# Solution Process

We decide to implement the lab with three solutions, implicit free lists, explicit free lists, and segregated free lists, which can be compared by the rate generated from `mdriver`.

The handouts given by the official website doesn't include trace files, which requests by `mdriver`. Run the `download_tracefile.py` to download needed files.

## Implicit free lists

Implicit free lists are the default solution given by the textbook, with partial codes shown in the book. Overall, we use first-fit, plus boundary marking, plus block merging, plus block splitting, to implement the heap distributor.

Since it searches for the fitted free block from the start, the time complexity of `mm_malloc` or `mm_realloc` is $O(n)$, which $n$ is the size of blocks in the heap. Meanwhile, the time complexity of `mm_free` is $O(1)$ because it only needs constant number of operations.

The test result is shown below:

```txt
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.004422  1288
 1       yes   99%    5848  0.004966  1178
 2       yes   99%    6648  0.006494  1024
 3       yes  100%    5380  0.005009  1074
 4       yes   66%   14400  0.000124116317
 5       yes   92%    4800  0.006152   780
 6       yes   92%    4800  0.006244   769
 7       yes   55%   12000  0.090600   132
 8       yes   51%   24000  0.175137   137
 9       yes   27%   14401  0.049681   290
10       yes   30%   14401  0.001338 10761
Total          74%  112372  0.350168   321

Perf index = 44 (util) + 21 (thru) = 66/100
```

## Explicit free lists

Obviously, neither graceful nor efficient the previous approach is, because the time taken is linearly related to the total number of blocks in the heap. Using explicit free lists can effectively improve this problem.

By linking free blocks in ascending order of size, we can use best-fit matching strategy to select the appropriate block, of which the time complexity is linearly related to the number of **free** blocks, not the total number of blocks in the heap.

Note that `MAX_HEAP` defined in `config.h`, indicating the maximum heap size in bytes being 20MB, is smaller than $2^{32}-1$. This means we can store the two pointers of the free block, the predecessor and the successor, with the format of the offset from the prologue pointer `heap_listp`, which only takes 4 bytes respectively whichever the OS is either 32bit or 64bit. As a result, the space utilization rate is consistent with the previous approach.

The test result is shown below:

```txt
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.000259 21976
 1       yes   99%    5848  0.000248 23543
 2       yes   99%    6648  0.000278 23957
 3       yes  100%    5380  0.000253 21248
 4       yes   66%   14400  0.000177 81172
 5       yes   96%    4800  0.004636  1035
 6       yes   95%    4800  0.004861   988
 7       yes   55%   12000  0.020663   581
 8       yes   51%   24000  0.096197   249
 9       yes   31%   14401  0.043109   334
10       yes   30%   14401  0.002198  6552
Total          75%  112372  0.172879   650

Perf index = 45 (util) + 40 (thru) = 85/100
```

## Segregated free lists

The cost time required by approach of explicit free lists depends on the number of free blocks in the heap. Each time we search for an eligible block, we have to start from the beginning. In the ascending order of size, it costs a lot for those searching for a block of large size.

Therefore, segregated free lists came into being. It shortens the search time by dividing the search into different ranges, which works on the same principle as skip lists. Here, I just roughly set five intervals, $[2^{4},2^{6})$, $[2^{6},2^{8})$, $[2^{8},2^{10})$, $[2^{10},2^{12})$, $[2^{12},\infty)$, and you can make further adjustments according to the data in the test file.

Although in the worst case, this solution will degenerate into explicit one, it will usually be better than the previous solution.

The common setup remains the same as before; we use an offset from `heap_listp` to represent the address of a block in the heap, in order to utilize the space.

The test result is shown below:

```txt
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.000202 28258
 1       yes   99%    5848  0.000194 30098
 2       yes   99%    6648  0.000249 26688
 3       yes  100%    5380  0.000190 28301
 4       yes   66%   14400  0.000315 45714
 5       yes   96%    4800  0.000813  5900
 6       yes   95%    4800  0.000777  6174
 7       yes   55%   12000  0.000357 33613
 8       yes   51%   24000  0.000852 28156
 9       yes   31%   14401  0.049653   290
10       yes   30%   14401  0.002553  5641
Total          75%  112372  0.056156  2001

Perf index = 45 (util) + 40 (thru) = 85/100
```

Don't be surprised by the result of no score improvement. You can improve it by simply adjusting the number of range divisions and the pattern. The important thing is the principle and method.
