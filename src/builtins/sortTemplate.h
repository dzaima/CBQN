// modified version of https://github.com/swenson/sort/tree/f79f2a525d03f102034b5a197c395f046eb82708
/* Copyright (c) 2010-2019 Christopher Swenson. */
/* Copyright (c) 2012 Vojtech Fried. */
/* Copyright (c) 2012 Google Inc. All Rights Reserved. */
/*
The MIT License (MIT)

Copyright (c) 2010-2019 Christopher Swenson and [others as listed in CONTRIBUTORS.md](CONTRIBUTORS.md)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

(contributors list, as this isn't markdown in the original repo: https://github.com/swenson/sort/blob/f79f2a525d03f102034b5a197c395f046eb82708/CONTRIBUTORS.md)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SMALL_SORT BINARY_INSERTION_SORT // we need stable sorting!!

#ifndef SORT_NAME
#error "Must declare SORT_NAME"
#endif

#ifndef SORT_TYPE
#error "Must declare SORT_TYPE"
#endif

#ifndef TIM_SORT_STACK_SIZE
#define TIM_SORT_STACK_SIZE 128
#endif

#ifndef SORT_SWAP
#define SORT_SWAP(x,y) {SORT_TYPE _sort_swap_temp = (x); (x) = (y); (y) = _sort_swap_temp;}
#endif

/* Common, type-agnostic functions and constants that we don't want to declare twice. */
#ifndef SORT_COMMON_H
#define SORT_COMMON_H

#ifndef MAX
#define MAX(x,y) (((x) > (y) ? (x) : (y)))
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y) ? (x) : (y)))
#endif

static inline int compute_minrun(const uint64_t size) {
  const int top_bit = 64 - CLZ(size);
  const int shift = MAX(top_bit, 6) - 6;
  const int minrun = (int)(size >> shift);
  const uint64_t mask = (1ULL << shift) - 1;
  if (mask & size) return minrun + 1;
  return minrun;
}

#endif /* SORT_COMMON_H */

#define SORT_CONCAT(x, y) x ## _ ## y
#define SORT_MAKE_STR1(x, y) SORT_CONCAT(x,y)
#define SORT_MAKE_STR(x) SORT_MAKE_STR1(SORT_NAME,x)

#ifndef SMALL_SORT_BND
#define SMALL_SORT_BND 16
#endif

#define SORT_TYPE_CPY                  SORT_MAKE_STR(sort_type_cpy)
#define BINARY_INSERTION_FIND          SORT_MAKE_STR(binary_insertion_find)
#define BINARY_INSERTION_SORT_START    SORT_MAKE_STR(binary_insertion_sort_start)
#define BINARY_INSERTION_SORT          SORT_MAKE_STR(binary_insertion_sort)
#define REVERSE_ELEMENTS               SORT_MAKE_STR(reverse_elements)
#define COUNT_RUN                      SORT_MAKE_STR(count_run)
#define CHECK_INVARIANT                SORT_MAKE_STR(check_invariant)
#define TIM_SORT                       SORT_MAKE_STR(tim_sort)
#define TIM_SORT_RESIZE                SORT_MAKE_STR(tim_sort_resize)
#define TIM_SORT_MERGE                 SORT_MAKE_STR(tim_sort_merge)
#define TIM_SORT_COLLAPSE              SORT_MAKE_STR(tim_sort_collapse)
#define TIM_SORT_RUN_T                 SORT_MAKE_STR(tim_sort_run_t)
#define TEMP_STORAGE_T                 SORT_MAKE_STR(temp_storage_t)
#define PUSH_NEXT                      SORT_MAKE_STR(push_next)

#ifndef MAX
#define MAX(x,y) (((x) > (y) ? (x) : (y)))
#endif
#ifndef MIN
#define MIN(x,y) (((x) < (y) ? (x) : (y)))
#endif
#ifndef SORT_CSWAP
#define SORT_CSWAP(x, y) { if(SORT_CMP((x),(y)) > 0) {SORT_SWAP((x),(y));}}
#endif

typedef struct {
  size_t start;
  size_t length;
} TIM_SORT_RUN_T;


void BINARY_INSERTION_SORT(SORT_TYPE *dst, const size_t size);


#undef SORT_TYPE_CPY
#define SORT_TYPE_CPY(dst, src, size) memcpy((dst), (src), (size) * sizeof(SORT_TYPE))




/* Function used to do a binary search for binary insertion sort */
static inline size_t BINARY_INSERTION_FIND(SORT_TYPE *dst, const SORT_TYPE x, const size_t size) {
  SORT_TYPE cx;
  size_t l = 0;
  size_t r = size - 1;
  size_t c = r >> 1;

  /* check for out of bounds at the beginning. */
  if (SORT_CMP(x, dst[0]) < 0) {
    return 0;
  } else if (SORT_CMP(x, dst[r]) > 0) {
    return r;
  }

  cx = dst[c];

  while (1) {
    if (SORT_CMP(x, cx) < 0) {
      if (c-l <= 1) return c;
      r = c;
    } else { /* allow = for stability. The binary search favors the right. */
      if (r-c <= 1) return c+1;
      l = c;
    }

    c = l + ((r-l) >> 1);
    cx = dst[c];
  }
}

/* Binary insertion sort, but knowing that the first "start" entries are sorted.  Used in timsort. */
static void BINARY_INSERTION_SORT_START(SORT_TYPE *dst, const size_t start, const size_t size) {
  for (size_t i = start; i < size; i++) {
    SORT_TYPE x;
    size_t location;

    /* If this entry is already correct, just move along */
    if (SORT_CMP(dst[i-1], dst[i]) <= 0) continue;

    /* Else we need to find the right place, shift everything over, and squeeze in */
    x = dst[i];
    location = BINARY_INSERTION_FIND(dst, x, i);

    for (size_t j = i-1; j >= location; j--) {
      dst[j + 1] = dst[j];
      if (j==0) break; // check edge case because j is unsigned
    }

    dst[location] = x;
  }
}

/* Binary insertion sort */
void BINARY_INSERTION_SORT(SORT_TYPE *dst, const size_t size) {
  if (size <= 1) return; // don't bother sorting an array of size <= 1
  BINARY_INSERTION_SORT_START(dst, 1, size);
}








/* timsort implementation, based on timsort.txt */

static inline void REVERSE_ELEMENTS(SORT_TYPE *dst, size_t start, size_t end) {
  while (1) {
    if (start >= end) return;

    SORT_SWAP(dst[start], dst[end]);
    start++;
    end--;
  }
}

static size_t COUNT_RUN(SORT_TYPE *dst, const size_t start, const size_t size) {
  if (size-start == 1) return 1;

  if (start >= size-2) {
    if (SORT_CMP(dst[size-2], dst[size-1]) > 0) {
      SORT_SWAP(dst[size-2], dst[size-1]);
    }

    return 2;
  }

  size_t curr = start + 2;

  if (SORT_CMP(dst[start], dst[start + 1]) <= 0) {
    /* increasing run */
    while (1) {
      if (curr == size-1) break;
      if (SORT_CMP(dst[curr-1], dst[curr]) > 0) break;
      curr++;
    }

    return curr-start;
  } else {
    /* decreasing run */
    while (1) {
      if (curr == size-1) break;
      if (SORT_CMP(dst[curr-1], dst[curr]) <= 0) break;
      curr++;
    }

    /* reverse in-place */
    REVERSE_ELEMENTS(dst, start, curr-1);
    return curr - start;
  }
}

static int CHECK_INVARIANT(TIM_SORT_RUN_T *stack, const int stack_curr) {
  if (stack_curr < 2) return 1;

  if (stack_curr == 2) {
    const size_t A1 = stack[stack_curr-2].length;
    const size_t B1 = stack[stack_curr-1].length;
    if (A1 <= B1) return 0;
    
    return 1;
  }

  size_t A = stack[stack_curr-3].length;
  size_t B = stack[stack_curr-2].length;
  size_t C = stack[stack_curr-1].length;
  if ((A <= B+C) || (B <= C)) return 0;

  return 1;
}

typedef struct {
  size_t alloc;
  SORT_TYPE *storage;
} TEMP_STORAGE_T;

static void TIM_SORT_RESIZE(TEMP_STORAGE_T *store, size_t new_size) {
  new_size*= 2;
  if (store->storage == NULL) {
    store->storage = TALLOCP(SORT_TYPE, new_size);
  } else if (store->alloc < new_size) {
    store->storage = (SORT_TYPE *)TREALLOC(store->storage, new_size * sizeof(SORT_TYPE));
  } else return;
  store->alloc = new_size;
}

static void TIM_SORT_MERGE(SORT_TYPE *dst, const TIM_SORT_RUN_T *stack, const int stack_curr,
                           TEMP_STORAGE_T *store) {
  const size_t X = stack[stack_curr-2].length;
  const size_t Y = stack[stack_curr-1].length;
  const size_t curr = stack[stack_curr-2].start;
  SORT_TYPE *storage;
  size_t i, j, k;
  TIM_SORT_RESIZE(store, MIN(X, Y));
  storage = store->storage;

  /* left merge */
  if (X < Y) {
    SORT_TYPE_CPY(storage, &dst[curr], X);
    i = 0;
    j = curr + X;

    for (k = curr; k < curr+X+Y; k++) {
      if ((i < X) && (j < curr+X+Y)) {
        if (SORT_CMP(storage[i], dst[j]) <= 0) {
          dst[k] = storage[i++];
        } else {
          dst[k] = dst[j++];
        }
      } else if (i < X) {
        dst[k] = storage[i++];
      } else {
        break;
      }
    }
  } else {
    /* right merge */
    SORT_TYPE_CPY(storage, &dst[curr + X], Y);
    i = Y;
    j = curr + X;
    k = curr + X + Y;

    while (k-- > curr) {
      if ((i > 0) && (j > curr)) {
        if (SORT_CMP(dst[j-1], storage[i-1]) > 0) {
          dst[k] = dst[--j];
        } else {
          dst[k] = storage[--i];
        }
      } else if (i > 0) {
        dst[k] = storage[--i];
      } else {
        break;
      }
    }
  }
}

static int TIM_SORT_COLLAPSE(SORT_TYPE *dst, TIM_SORT_RUN_T *stack, int stack_curr,
                             TEMP_STORAGE_T *store, const size_t size) {
  while (1) {
    size_t A, B, C, D;
    int ABC, BCD, CD;

    /* if the stack only has one thing on it, we are done with the collapse */
    if (stack_curr <= 1) {
      break;
    }

    /* if this is the last merge, just do it */
    if ((stack_curr == 2) && (stack[0].length + stack[1].length == size)) {
      TIM_SORT_MERGE(dst, stack, stack_curr, store);
      stack[0].length += stack[1].length;
      stack_curr--;
      break;
    }
    /* check if the invariant is off for a stack of 2 elements */
    else if ((stack_curr == 2) && (stack[0].length <= stack[1].length)) {
      TIM_SORT_MERGE(dst, stack, stack_curr, store);
      stack[0].length += stack[1].length;
      stack_curr--;
      break;
    } else if (stack_curr == 2) {
      break;
    }

    B = stack[stack_curr-3].length;
    C = stack[stack_curr-2].length;
    D = stack[stack_curr-1].length;

    if (stack_curr >= 4) {
      A = stack[stack_curr-4].length;
      ABC = (A <= B + C);
    } else {
      ABC = 0;
    }

    BCD = (B <= C + D) || ABC;
    CD = (C <= D);

    /* Both invariants are good */
    if (!BCD && !CD) break;

    /* left merge */
    if (BCD && !CD) {
      TIM_SORT_MERGE(dst, stack, stack_curr - 1, store);
      stack[stack_curr-3].length += stack[stack_curr-2].length;
      stack[stack_curr-2] = stack[stack_curr-1];
      stack_curr--;
    } else {
      /* right merge */
      TIM_SORT_MERGE(dst, stack, stack_curr, store);
      stack[stack_curr-2].length += stack[stack_curr-1].length;
      stack_curr--;
    }
  }

  return stack_curr;
}

static inline int PUSH_NEXT(SORT_TYPE *dst,
                              const size_t size,
                              TEMP_STORAGE_T *store,
                              const size_t minrun,
                              TIM_SORT_RUN_T *run_stack,
                              size_t *stack_curr,
                              size_t *curr) {
  size_t len = COUNT_RUN(dst, *curr, size);
  size_t run = minrun;

  if (run > size - *curr) {
    run = size - *curr;
  }

  if (run > len) {
    BINARY_INSERTION_SORT_START(&dst[*curr], len, run);
    len = run;
  }

  run_stack[*stack_curr].start = *curr;
  run_stack[*stack_curr].length = len;
  (*stack_curr)++;
  *curr += len;

  if (*curr == size) {
    /* finish up */
    while (*stack_curr > 1) {
      TIM_SORT_MERGE(dst, run_stack, (int)*stack_curr, store);
      run_stack[*stack_curr - 2].length += run_stack[*stack_curr - 1].length;
      (*stack_curr)--;
    }

    if (store->storage != NULL) {
      TFREE(store->storage);
      store->storage = NULL;
    }

    return 0;
  }

  return 1;
}

static inline void TIM_SORT(SORT_TYPE *dst, const size_t size) {
  size_t minrun;
  TEMP_STORAGE_T _store, *store;
  TIM_SORT_RUN_T run_stack[TIM_SORT_STACK_SIZE];
  size_t stack_curr = 0;
  size_t curr = 0;

  /* don't bother sorting an array of size 1 */
  if (size <= 1) return;

  if (size < 64) {
    SMALL_SORT(dst, size);
    return;
  }

  /* compute the minimum run length */
  minrun = compute_minrun(size);
  /* temporary storage for merges */
  store = &_store;
  store->alloc = 0;
  store->storage = NULL;

  if (!PUSH_NEXT(dst, size, store, minrun, run_stack, &stack_curr, &curr)) return;
  if (!PUSH_NEXT(dst, size, store, minrun, run_stack, &stack_curr, &curr)) return;
  if (!PUSH_NEXT(dst, size, store, minrun, run_stack, &stack_curr, &curr)) return;
  
  while (1) {
    if (!CHECK_INVARIANT(run_stack, (int)stack_curr)) {
      stack_curr = TIM_SORT_COLLAPSE(dst, run_stack, (int)stack_curr, store, size);
      continue;
    }

    if (!PUSH_NEXT(dst, size, store, minrun, run_stack, &stack_curr, &curr)) return;
  }
}



#undef SORT_TYPE_CPY
#undef SORT_CONCAT
#undef SORT_MAKE_STR1
#undef SORT_MAKE_STR
#undef SORT_NAME
#undef SORT_TYPE
#undef SORT_CMP
#undef TEMP_STORAGE_T
#undef TIM_SORT_RUN_T
#undef PUSH_NEXT
#undef SORT_SWAP
#undef BINARY_INSERTION_FIND
#undef BINARY_INSERTION_SORT_START
#undef BINARY_INSERTION_SORT
#undef REVERSE_ELEMENTS
#undef COUNT_RUN
#undef TIM_SORT
#undef TIM_SORT_RESIZE
#undef TIM_SORT_COLLAPSE
