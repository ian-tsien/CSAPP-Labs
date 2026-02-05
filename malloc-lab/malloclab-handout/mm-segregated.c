/*
 * mm-segregated.c - Use segregated free list as the approach.
 * 1. Use segregated free lists to record free blocks.
 * 2. After a free block is occupied, the remaining portion needs to be
 * processed.
 * 3. After freeing a block, the block needs to be merged with adjacent free
 * blocks.
 * 4. Choose best adaptation as the input strategy.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "solo",
    /* First member's full name */
    "ian",
    /* First member's email address */
    "ian.tsien.contact@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and fotter */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given free block ptr bp, compute the offset from the base */
#define GET_OFFSET(bp, base) ((char *)(bp) - (char *)(base))

/* Given base and offset, compute the address */
#define GET_ADDRESS(offset, base) (())

/* Given free block ptr bp, compute address of next and previous free blocks */
#define NEXT_FREE_BLKP(bp, base) ((char *)(base) + GET(bp + WSIZE))
#define PREV_FREE_BLKP(bp, base) ((char *)(base) + GET(bp))

static void *heap_listp; /* Heap header */

/* Function declaration */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void add_free_block(void *bp);
static void remove_free_block(void *bp);
static void *find_list(size_t size);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  /* Create the initial empty heap */
  if ((heap_listp = mem_sbrk(7 * DSIZE)) == (void *)-1) {
    return -1;
  }
  PUT(heap_listp, 0);                       /* the predecessor of [2^4,2^6) */
  PUT(heap_listp + (1 * WSIZE), 0);         /* the successor of [2^4,2^6) */
  PUT(heap_listp + (2 * WSIZE), DSIZE);     /* the predecessor of [2^6,2^8) */
  PUT(heap_listp + (3 * WSIZE), DSIZE);     /* the successor of [2^6,2^8) */
  PUT(heap_listp + (4 * WSIZE), 2 * DSIZE); /* the predecessor of [2^8,2^10) */
  PUT(heap_listp + (5 * WSIZE), 2 * DSIZE); /* the successor of [2^8,2^10) */
  PUT(heap_listp + (6 * WSIZE), 3 * DSIZE); /* the predecessor of [2^10,2^12) */
  PUT(heap_listp + (7 * WSIZE), 3 * DSIZE); /* the successor of [2^10,2^12) */
  PUT(heap_listp + (8 * WSIZE), 4 * DSIZE); /* the predecessor of [2^12,inf) */
  PUT(heap_listp + (9 * WSIZE), 4 * DSIZE); /* the successor of [2^12,inf) */
  PUT(heap_listp + (10 * WSIZE), 0);        /* Alignment padding */
  PUT(heap_listp + (11 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (12 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(heap_listp + (13 * WSIZE), PACK(0, 1));     /* Epilogue header */

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
    return -1;
  }
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;

  /* Ignore spurious requests */
  if (size == 0) {
    return NULL;
  }

  /* Adjust block size to include overhead and alignment reqs. */
  asize = ALIGN(size) + DSIZE;

  /* Search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
    return NULL;
  }
  place(bp, asize);
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *newptr;

  if (ptr ==
      NULL) { /* If ptr is NULL, the call is equivalent to mm_malloc(size) */
    return mm_malloc(size);
  }
  if (size == 0) { /* If size is equal to zero, the call is equivalent to
                      mm_free(ptr) */
    mm_free(ptr);
    return NULL;
  }

  /* Add a new block */
  if ((newptr = mm_malloc(size)) == NULL) {
    return NULL;
  }
  memcpy(newptr, ptr, MIN(GET_SIZE(HDRP(ptr)) - DSIZE, size));
  mm_free(ptr);
  return newptr;
}

/*
 * extend_heap - Extend the heap with a free block of words words.
 */
static void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
  PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

  /* Coalesce if the previous block was free */
  return coalesce(bp);
}

/*
 * coalesce - Coalesce adjacent free blocks.
 */
static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) { /* Case 1 */
    // do nothing here
  } else if (prev_alloc && !next_alloc) { /* Case 2 */
    remove_free_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  } else if (!prev_alloc && next_alloc) { /* Case 3 */
    remove_free_block(PREV_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  } else { /* Case 4 */
    remove_free_block(NEXT_BLKP(bp));
    remove_free_block(PREV_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  add_free_block(bp);
  return bp;
}

/*
 * find_fit - Find the best free block which can accommodate size bytes.
 */
static void *find_fit(size_t size) {
  void *list_header;
  void *bp;

  for (list_header = find_list(size); list_header <= (heap_listp + 4 * DSIZE);
       list_header += DSIZE) {
    for (bp = NEXT_FREE_BLKP(list_header, heap_listp); bp != list_header;
         bp = NEXT_FREE_BLKP(bp, heap_listp)) {
      if (GET_SIZE(HDRP(bp)) >= size) {
        return bp;
      }
    }
  }
  return NULL;
}

/*
 * place - Sotre data into the free block.
 */
static void place(void *bp, size_t size) {
  size_t capacity = GET_SIZE(HDRP(bp));

  remove_free_block(bp);
  /* Determine if this block can be cut */
  if ((capacity - size) >= 2 * DSIZE) {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(capacity - size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(capacity - size, 0));
    coalesce(NEXT_BLKP(bp));
  } else {
    PUT(HDRP(bp), PACK(capacity, 1));
    PUT(FTRP(bp), PACK(capacity, 1));
  }
}

/*
 * add_free_block - Add a free block into the segregated free lists.
 */
static void add_free_block(void *bp) {
  size_t size = GET_SIZE(HDRP(bp));
  size_t offset = GET_OFFSET(bp, heap_listp);
  char *list_header = find_list(size);
  char *ptr = list_header;
  char *next_ptr = NEXT_FREE_BLKP(ptr, heap_listp);
  size_t next_size = GET_SIZE(HDRP(next_ptr));

  while ((next_ptr != list_header) && (next_size < size)) {
    ptr = next_ptr;
    next_ptr = NEXT_FREE_BLKP(ptr, heap_listp);
    next_size = GET_SIZE(HDRP(next_ptr));
  }

  PUT(bp, GET(next_ptr));
  PUT(bp + WSIZE, GET(ptr + WSIZE));
  PUT(next_ptr, offset);
  PUT(ptr + WSIZE, offset);
}

/*
 * remove_free_block - Remove a free block from the segregated free lists.
 */
static void remove_free_block(void *bp) {
  PUT(PREV_FREE_BLKP(bp, heap_listp) + WSIZE, GET(bp + WSIZE));
  PUT(NEXT_FREE_BLKP(bp, heap_listp), GET(bp));
}

/*
 * find_list - Find which list can hold the block of this size.
 */
static void *find_list(size_t size) {
  int msb = 0;
  int bit12, bit10, bit8, bit6;
  int offset;

  while (size >>= 1) {
    msb++;
  }

  bit12 = (msb > 12);
  bit10 = (msb > 10) & !bit12;
  bit8 = (msb > 8) & !bit12 & !bit10;
  bit6 = (msb > 6) & !bit12 & !bit10 & !bit8;
  offset = bit12 * 4 + bit10 * 3 + bit8 * 2 + bit6 * 1;
  return heap_listp + offset * DSIZE;
}
