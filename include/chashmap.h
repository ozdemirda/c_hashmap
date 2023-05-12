/*
MIT License

Copyright (c) 2018 Danis Ozdemir

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
*/

#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>

typedef struct chashmap chashmap;

typedef struct chmap_pair {
  void* ptr;
  uint32_t size;
} chmap_pair;

// The function 'chmap_create' creates a new hash map instance and returns
// the pointer to it. This pointer should be passed to the macro
// 'chmap_destroy', once the hash map is no longer needed. The input parameter
// 'bucket_array_size' determines the initial bucket size of the hash map.
chashmap* chmap_create(uint32_t initial_bucket_array_size);

// The function 'chmap_elem_count' can be used to get the number of elements
// in it. If chmap is NULL, this function will return 0;
uint32_t chmap_elem_count(chashmap* chmap);

// The function 'chmap_reset' can be used to clear a map by deleting the
// existing elements from it. If the new_bucket_array_size is 0, the
// bucket array size will not change, otherwise, the bucket array will
// get reallocated to match the specified size. If that reallocation fails,
// this function will return -1. If the chmap pointer is not NULL, it will
// always destroy the existing elements regardless of its return value,
// the return value conveys information regarding the realloc attempt.
int chmap_reset(chashmap* chmap, uint32_t new_bucket_array_size);

// The function chmap_insert_elem 'upserts' the val_pair associated with
// the key_pair into the hash map chmap. If no element exists in the map
// associated with the provided key_pair, this function will return -1,
// otherwise it will return 0.
int chmap_insert_elem(chashmap* chmap, const chmap_pair* key_pair,
                      const chmap_pair* val_pair);

// The function chmap_get_elem_copy populates a copy of the original val_pair
// from the hash map into the val_pair. val_pair SHOULD BE non-null, otherwise
// the function will fail. This function is slower than the function
// 'chmap_get_elem_ref', and later modifications to val_pair are not reflected
// in the map's copy, but it's safer.
int chmap_get_elem_copy(chashmap* chmap, const chmap_pair* key_pair,
                        void* target_buf, uint32_t target_buf_size);

// This function gets a pointer to the element's value pair and stores it into
// the pointer-to-pointer val_pair. Here, naturally, val_pair gets overwritten.
// USE THIS FUNCTION WITH CAUTION, AS THE POINTER'S VALIDITY IS ONLY GUARANTEED
// WHILE THIS FUNCTION IS RUNNING AND THE POINTER MAY GET DEALLOCATED ANY TIME
// AFTER THIS FUNCTION'S COMPLETION. IT MAY COME HANDY FOR THE SCENARIOS
// THAT THE CALLER KNOWS FOR SURE THAT THE ELEMENT IS NOT GOING TO BE FREED
// ANY TIME SOON.
// USING THE FUNCTION chmap_exec_func_on_elem INSTEAD OF THIS ONE
// IS STRONGLY RECOMMENDED. CONSIDER YOURSELF WARNED.
int chmap_get_elem_ref(chashmap* chmap, const chmap_pair* key_pair,
                       chmap_pair** val_pair);

// The function 'chmap_delete_elem' can be used to delete an element from the
// hash map.
void chmap_delete_elem(chashmap* chmap, const chmap_pair* key_pair);

// The function 'chmap_exec_func_on_elem' can be used to perform incremental
// modifications on the val_pair for a given key_pair. It's safer, because
// during its execution, the synchronization locks are held. PLEASE NOTICE
// THAT THE CALLBACK FUNCTION SHOULD BE FAST, AND IT SHOULD NOT MAKE
// ANY BLOCKING CALLS, AS THIS MAY DEGRADE THE PERFORMANCE, IT MAY EVEN
// CAUSE DEADLOCKS DEPENDING WHAT'S DONE IN IT. FINALLY, PLEASE NOTICE THE
// 'const' FOR THE key_pair IN THE CALLBACK'S SIGNATURE, IF YOU NEGLECT IT
// USING SOME CASTING AND MODIFY THE KEY, YOU'LL SHOOT YOURSELF ON THE FOOT.
int chmap_exec_func_on_elem(chashmap* chmap, const chmap_pair* key_pair,
                            void (*callback)(const chmap_pair* key_pair,
                                             chmap_pair* val_pair, void* args),
                            void* args);

// The function 'chmap_for_each_elem_wr' is meant to provide a mechanism similar
// to iteration. It will write-lock the hash map and execute the callback on the
// every element present in the map. PLEASE DO NOT NEGLECT THE 'const' FOR
// THE 'key_pair' IN THE CALLBACK, AND TAMPER WITH IT. THAT WILL CAUSE
// PROBLEMS.
void chmap_for_each_elem_wr(chashmap* chmap,
                            void (*callback)(const chmap_pair* key_pair,
                                             chmap_pair* val_pair, void* args),
                            void* args);

// The function 'chmap_for_each_elem_rd' is meant to provide a mechanism similar
// to iteration. It will read-lock the hash map and execute the callback on the
// every element present in the map. PLEASE DO NOT NEGLECT THE 'const'
// specifiers FOR THE 'key_pair' AND 'val_pair' IN THE CALLBACK, AND TAMPER
// WITH THEM. THAT WILL NATURALLY CAUSE PROBLEMS.
void chmap_for_each_elem_rd(chashmap* chmap,
                            void (*callback)(const chmap_pair* key_pair,
                                             const chmap_pair* val_pair,
                                             void* args),
                            void* args);

// The function '_chmap_destroy' is not meant to be used directly, please use
// the macro 'chmap_destroy' instead.
void _chmap_destroy(chashmap* chmap);

// The macro 'chmap_destroy' can be used to destroy a hash map. The pointer
// then gets set to NULL.
#define chmap_destroy(chmap) \
  do {                       \
    _chmap_destroy(chmap);   \
    chmap = NULL;            \
  } while (0)
