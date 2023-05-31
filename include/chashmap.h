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

// The function chmap_get_elem_copy populates a copy of the data stored in the
// val_pair from the hash map into the target_buf. The pointer target_buf SHOULD
// BE non-null, otherwise the function will fail.
int chmap_get_elem_copy(chashmap* chmap, const chmap_pair* key_pair,
                        void* target_buf, uint32_t target_buf_size);

// This function gets a pointer to the element's value pair and stores it into
// the pointer-to-pointer val_pair. Here, naturally, val_pair gets overwritten.
int chmap_get_elem_ref(chashmap* chmap, const chmap_pair* key_pair,
                       chmap_pair** val_pair);

// The function 'chmap_delete_elem' can be used to delete an element from the
// hash map.
void chmap_delete_elem(chashmap* chmap, const chmap_pair* key_pair);

// The function 'chmap_for_each_elem' is meant to provide a mechanism similar
// to iteration. It will execute the callback on the every element present in
// the map. PLEASE DO NOT NEGLECT THE 'const' FOR THE 'key_pair' IN THE
// CALLBACK, AND TAMPER WITH IT. THAT WILL CAUSE PROBLEMS.
void chmap_for_each_elem(chashmap* chmap,
                         void (*callback)(const chmap_pair* key_pair,
                                          chmap_pair* val_pair, void* args),
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
