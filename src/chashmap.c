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

#include <chashmap.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

const uint32_t minimum_allowed_bucket_array_size = 2048;
const uint32_t scale_factor = 4;
const uint32_t minimum_scale_down_threshold =
    scale_factor * minimum_allowed_bucket_array_size;

typedef struct chmap_entry {
  unsigned long hash_val;
  chmap_pair* key_pair;
  chmap_pair* val_pair;
} chmap_entry;

typedef struct llist_node llist_node;

typedef struct dllist_ref_node {
  // All these pointers are mere references,
  // and they are not 'owned' by this struct
  struct dllist_ref_node* prev;
  struct dllist_ref_node* next;
  llist_node* host;
} dllist_ref_node;

void attach_node_to_dllist(dllist_ref_node** head, dllist_ref_node* node,
                           llist_node* host) {
  node->prev = NULL;
  node->host = host;
  if (!(*head)) {
    node->next = NULL;
    *head = node;
  } else {
    node->next = *head;
    (*head)->prev = node;
    *head = node;
  }
}

void detach_node_from_dllist(dllist_ref_node** head, dllist_ref_node* node) {
  if (node->next) {
    node->next->prev = node->prev;
  }

  if (node->prev) {
    node->prev->next = node->next;
  }

  if (*head == node) {
    *head = node->next;
  }
}

struct llist_node {
  struct llist_node* next;
  dllist_ref_node dllist_refs;
  chmap_entry data;
};

void destroy_llist_node(dllist_ref_node** head_of_all_elems, llist_node* elem) {
  if (elem) {
    if (elem->data.key_pair->ptr) free(elem->data.key_pair->ptr);
    if (elem->data.key_pair) free(elem->data.key_pair);
    if (elem->data.val_pair->ptr) free(elem->data.val_pair->ptr);
    if (elem->data.val_pair) free(elem->data.val_pair);
    if (head_of_all_elems) {
      detach_node_from_dllist(head_of_all_elems, &elem->dllist_refs);
    }
    free(elem);
  }
}

llist_node* create_llist_node(dllist_ref_node** head_of_all_elems,
                              chmap_entry* data) {
  llist_node* new_elem = (llist_node*)calloc(1, sizeof(llist_node));
  if (!new_elem) {
    return NULL;
  }

  new_elem->data.key_pair = (chmap_pair*)malloc(sizeof(chmap_pair));
  if (!new_elem->data.key_pair) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  new_elem->data.val_pair = (chmap_pair*)malloc(sizeof(chmap_pair));
  if (!new_elem->data.val_pair) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  new_elem->data.key_pair->ptr = malloc(data->key_pair->size);
  if (!new_elem->data.key_pair->ptr) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  new_elem->data.val_pair->ptr = malloc(data->val_pair->size);
  if (!new_elem->data.val_pair->ptr) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  attach_node_to_dllist(head_of_all_elems, &new_elem->dllist_refs, new_elem);
  new_elem->next = NULL;
  new_elem->data.hash_val = data->hash_val;
  new_elem->data.key_pair->size = data->key_pair->size;
  new_elem->data.val_pair->size = data->val_pair->size;
  memcpy(new_elem->data.key_pair->ptr, data->key_pair->ptr,
         data->key_pair->size);
  memcpy(new_elem->data.val_pair->ptr, data->val_pair->ptr,
         data->val_pair->size);

  return new_elem;
}

bool reset_val_of_llist_node(llist_node* elem, const chmap_pair* val_pair) {
  bool success = false;

  if (elem->data.val_pair->size == val_pair->size) {
    // The new value has the same size
    memcpy(elem->data.val_pair->ptr, val_pair->ptr, val_pair->size);
    success = true;
  } else {
    // Sizes do not match, trying to realloc.
    void* orig_buf = elem->data.val_pair->ptr;
    elem->data.val_pair->ptr =
        realloc(elem->data.val_pair->ptr, val_pair->size);
    if (!elem->data.val_pair->ptr) {
      // Failed to reallocate.
      elem->data.val_pair->ptr = orig_buf;
    } else {
      // Buffer reallocated, all is good.
      memcpy(elem->data.val_pair->ptr, val_pair->ptr, val_pair->size);
      elem->data.val_pair->size = val_pair->size;
      success = true;
    }
  }

  return success;
}

llist_node* insert_into_llist(llist_node* head,
                              dllist_ref_node** head_of_all_elems,
                              chmap_entry* data, bool* success) {
  llist_node* new_elem = create_llist_node(head_of_all_elems, data);
  if (!new_elem) {
    *success = false;
    return head;
  }

  *success = true;

  new_elem->next = head;
  head = new_elem;

  return head;
}

llist_node* migrate_llist_node_to_another_llist(llist_node* head,
                                                llist_node* prev_node,
                                                llist_node* node) {
  if (prev_node) {
    prev_node->next = node->next;
  }

  node->next = head;
  head = node;

  return head;
}

static inline bool compare_key_pairs(const chmap_pair* kp1,
                                     const chmap_pair* kp2) {
  if (kp1->size != kp2->size) {
    return false;
  }

  if (kp1->size == sizeof(int) &&
      *(unsigned int*)kp1->ptr == *(unsigned int*)kp2->ptr) {
    return true;
  } else if (kp1->size == sizeof(long) &&
             *(unsigned long*)kp1->ptr == *(unsigned long*)kp2->ptr) {
    return true;
  } else if (kp1->size == sizeof(char) &&
             *(unsigned char*)kp1->ptr == *(unsigned char*)kp2->ptr) {
    return true;
  } else if (kp1->size == sizeof(short) &&
             *(unsigned short*)kp1->ptr == *(unsigned short*)kp2->ptr) {
    return true;
  } else if (memcmp(kp1->ptr, kp2->ptr, kp1->size) == 0) {
    return true;
  }

  return false;
}

llist_node* find_in_llist(llist_node* head, const chmap_pair* key_pair) {
  llist_node* tracker = head;
  while (tracker) {
    if (compare_key_pairs(tracker->data.key_pair, key_pair)) {
      return tracker;
    }
    tracker = tracker->next;
  }

  return tracker;
}

llist_node* destroy_the_whole_llist(llist_node* head,
                                    dllist_ref_node** head_of_all_elems) {
  llist_node* tracker = head;

  while (tracker) {
    llist_node* node_to_be_deleted = tracker;
    tracker = tracker->next;
    destroy_llist_node(head_of_all_elems, node_to_be_deleted);
  }

  return tracker;
}

llist_node* delete_from_llist(llist_node* head,
                              dllist_ref_node** head_of_all_elems,
                              const chmap_pair* key_pair, bool* found) {
  *found = false;
  llist_node* tracker = head;
  llist_node* previous = NULL;

  while (tracker) {
    if (tracker->data.key_pair->size == key_pair->size) {
      if (compare_key_pairs(tracker->data.key_pair, key_pair)) {
        // This is the node to be deleted
        *found = true;
        if (!previous) {
          head = tracker->next;
        } else {
          previous->next = tracker->next;
        }
        destroy_llist_node(head_of_all_elems, tracker);

        return head;
      }
    }

    previous = tracker;
    tracker = tracker->next;
  }

  return head;
}

struct chashmap {
  pthread_rwlock_t lock;
  uint32_t elem_count;
  uint32_t bucket_arr_size;
  uint32_t elem_count_to_scale_up;
  uint32_t elem_count_to_scale_down;
  llist_node** bucket_arr;
  dllist_ref_node* head_of_all_elems;
};

void set_chmap_scaling_limits(chashmap* chmap) {
  chmap->elem_count_to_scale_up = chmap->bucket_arr_size * 6 / 4;
  chmap->elem_count_to_scale_down = chmap->bucket_arr_size / 8;
}

chashmap* chmap_create(uint32_t initial_bucket_array_size) {
  if (initial_bucket_array_size == 0) {
    return NULL;
  }

  if (initial_bucket_array_size < minimum_allowed_bucket_array_size) {
    initial_bucket_array_size = minimum_allowed_bucket_array_size;
  } else {
    initial_bucket_array_size =
        powl(2, ceill(log2l(initial_bucket_array_size)));
  }

  chashmap* chmap = (chashmap*)malloc(sizeof(chashmap));
  if (!chmap) {
    // Failed to allocate buffer
    fprintf(stderr, "%s - %s:%d - Failed to allocate buffer\n",
            __PRETTY_FUNCTION__, __FILE__, __LINE__);
    return NULL;
  }

  if (pthread_rwlock_init(&chmap->lock, NULL)) {
    // Failed to initialize map mutex
    fprintf(stderr, "%s - %s:%d - Failed to initialize map mutex\n",
            __PRETTY_FUNCTION__, __FILE__, __LINE__);
    free(chmap);
    return NULL;
  }

  chmap->bucket_arr_size = initial_bucket_array_size;
  chmap->elem_count = 0;
  chmap->head_of_all_elems = NULL;
  set_chmap_scaling_limits(chmap);
  chmap->bucket_arr =
      (llist_node**)calloc(initial_bucket_array_size, sizeof(llist_node*));

  if (!chmap->bucket_arr) {
    // Failed to allocate buffer for bucket_arr
    fprintf(stderr, "%s - %s:%d - Failed to allocate bucket_arr\n",
            __PRETTY_FUNCTION__, __FILE__, __LINE__);
    pthread_rwlock_destroy(&chmap->lock);
    free(chmap);
    return NULL;
  }

  return chmap;
}

static inline void assign_key_to_hash_id(unsigned long* id_ptr, uint32_t size,
                                         const unsigned char* c_key_ptr) {
  if (size == sizeof(unsigned int)) {
    *id_ptr = *(unsigned int*)c_key_ptr;
  } else if (size == sizeof(unsigned long)) {
    *id_ptr = *(unsigned long*)c_key_ptr;
  } else if (size == sizeof(unsigned char)) {
    *id_ptr = *c_key_ptr;
  } else if (size == sizeof(unsigned short)) {
    *id_ptr = *(unsigned short*)c_key_ptr;
  } else {
    unsigned char* c_id_ptr = (unsigned char*)id_ptr;
#if BYTE_ORDER == LITTLE_ENDIAN
    for (uint32_t i = 0; i < size; ++i) {
      c_id_ptr[i] = c_key_ptr[i];
    }
#else
    uint32_t offset = sizeof(id) - size;
    for (uint32_t i = 0; i < size; ++i) {
      c_id_ptr[offset + i] = c_key_ptr[i];
    }
#endif
  }
}

static inline uint32_t calculate_bucket_index(uint32_t bucket_arr_size,
                                              const chmap_pair* key_pair,
                                              unsigned long* hash_ptr) {
  unsigned long id = 0x0;

  unsigned char* c_key_ptr = (unsigned char*)key_pair->ptr;
  uint32_t size = key_pair->size;

  if (size <= sizeof(id)) {
    // Kind of a number assignment.
    assign_key_to_hash_id(&id, size, c_key_ptr);
  } else {
    id = (2 * c_key_ptr[0] + 1);

    for (uint32_t i = 1; i < size; ++i) {
      if (i % 2 == 0) {
        id += (2 * c_key_ptr[i] + 1);
      } else {
        id ^= (2 * c_key_ptr[i] + 1);
      }
    }
  }

  if (hash_ptr) {
    *hash_ptr = id;
  }

  uint32_t index = (id % bucket_arr_size);

  return index;
}

#ifdef RUNNING_UNIT_TESTS
uint32_t chmap_get_bucket_arr_size(chashmap* chmap) {
  if (!chmap) {
    return 0;
  }

  uint32_t r;

  pthread_rwlock_rdlock(&chmap->lock);
  r = chmap->bucket_arr_size;
  pthread_rwlock_unlock(&chmap->lock);

  return r;
}

uint32_t chmap_get_elem_count_to_scale_up(chashmap* chmap) {
  if (!chmap) {
    return 0;
  }

  uint32_t r;

  pthread_rwlock_rdlock(&chmap->lock);
  r = chmap->elem_count_to_scale_up;
  pthread_rwlock_unlock(&chmap->lock);

  return r;
}

uint32_t chmap_get_elem_count_to_scale_down(chashmap* chmap) {
  if (!chmap) {
    return 0;
  }

  uint32_t r;

  pthread_rwlock_rdlock(&chmap->lock);
  r = chmap->elem_count_to_scale_down;
  pthread_rwlock_unlock(&chmap->lock);

  return r;
}
#endif

// This function is called while holding the chmap mutex, so no further
// locking is needed.
void scale_chmap(chashmap* chmap, bool up) {
  uint32_t new_bucket_array_size = 0;
  if (up) {
    new_bucket_array_size = chmap->bucket_arr_size * scale_factor;
  } else {
    new_bucket_array_size = chmap->bucket_arr_size / scale_factor;
  }

  llist_node** new_bucket_arr =
      (llist_node**)calloc(new_bucket_array_size, sizeof(llist_node*));
  if (!new_bucket_arr) {
    // We don't have enough memory to scale, return.
    return;
  }

  // We have enough memory.
  for (uint32_t i = 0; i < chmap->bucket_arr_size; ++i) {
    llist_node* tracker = chmap->bucket_arr[i];
    while (tracker) {
      llist_node* next = tracker->next;
      uint32_t new_index = tracker->data.hash_val % new_bucket_array_size;
      new_bucket_arr[new_index] = migrate_llist_node_to_another_llist(
          new_bucket_arr[new_index], NULL, tracker);
      tracker = next;
    }
    chmap->bucket_arr[i] = NULL;
  }

  free(chmap->bucket_arr);
  chmap->bucket_arr = new_bucket_arr;
  chmap->bucket_arr_size = new_bucket_array_size;
  set_chmap_scaling_limits(chmap);
}

int chmap_insert_elem(chashmap* chmap, const chmap_pair* key_pair,
                      const chmap_pair* val_pair) {
  if (!chmap || !key_pair || !val_pair || !key_pair->ptr || !val_pair->ptr ||
      key_pair->size == 0 || val_pair->size == 0) {
    return -1;
  }

  chmap_entry data = {.hash_val = 0,
                      .key_pair = (chmap_pair*)key_pair,
                      .val_pair = (chmap_pair*)val_pair};

  bool success = false;

  pthread_rwlock_wrlock(&chmap->lock);

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, &data.hash_val);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    // The entry already exists
    success = reset_val_of_llist_node(r, val_pair);
  } else {
    chmap->bucket_arr[index] = insert_into_llist(
        chmap->bucket_arr[index], &chmap->head_of_all_elems, &data, &success);
    if (success) {
      if (++chmap->elem_count >= chmap->elem_count_to_scale_up) {
        // Time to scale up!
        scale_chmap(chmap, true);
      }
    }
  }

  pthread_rwlock_unlock(&chmap->lock);

  return success ? 0 : -1;
}

static inline void mem_assign(void* dest, void* src, uint32_t size) {
  if (size == sizeof(unsigned int)) {
    *(unsigned int*)dest = *(unsigned int*)src;
  } else if (size == sizeof(unsigned long)) {
    *(unsigned long*)dest = *(unsigned long*)src;
  } else if (size == sizeof(unsigned char)) {
    *(unsigned char*)dest = *(unsigned char*)src;
  } else if (size == sizeof(unsigned short)) {
    *(unsigned short*)dest = *(unsigned short*)src;
  } else {
    memcpy(dest, src, size);
  }
}

int chmap_get_elem_copy(chashmap* chmap, const chmap_pair* key_pair,
                        void* target_buf, uint32_t target_buf_size) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0 ||
      !target_buf || target_buf_size == 0) {
    return -1;
  }

  int success = -1;

  pthread_rwlock_rdlock(&chmap->lock);

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, NULL);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    uint32_t min_size = target_buf_size;
    if (r->data.val_pair->size < min_size) {
      min_size = r->data.val_pair->size;
    }
    mem_assign(target_buf, r->data.val_pair->ptr, min_size);
    success = 0;
  }

  pthread_rwlock_unlock(&chmap->lock);

  return success;
}

int chmap_get_elem_ref(chashmap* chmap, const chmap_pair* key_pair,
                       chmap_pair** val_pair) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0 ||
      !val_pair) {
    return -1;
  }

  int success = -1;

  pthread_rwlock_rdlock(&chmap->lock);

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, NULL);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    *val_pair = r->data.val_pair;
    success = 0;
  }

  pthread_rwlock_unlock(&chmap->lock);

  return success;
}

int chmap_exec_func_on_elem(chashmap* chmap, const chmap_pair* key_pair,
                            void (*callback)(const chmap_pair* key_pair,
                                             chmap_pair* val_pair, void* args),
                            void* args) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0 ||
      !callback) {
    return -1;
  }

  int success = -1;

  pthread_rwlock_wrlock(&chmap->lock);

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, NULL);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    (*callback)(r->data.key_pair, r->data.val_pair, args);
    success = 0;
  }

  pthread_rwlock_unlock(&chmap->lock);

  return success;
}

void chmap_delete_elem(chashmap* chmap, const chmap_pair* key_pair) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0) {
    return;
  }

  bool found = false;

  pthread_rwlock_wrlock(&chmap->lock);

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, NULL);

  chmap->bucket_arr[index] = delete_from_llist(
      chmap->bucket_arr[index], &chmap->head_of_all_elems, key_pair, &found);

  if (found) {
    if (--chmap->elem_count < chmap->elem_count_to_scale_down &&
        chmap->bucket_arr_size >= minimum_scale_down_threshold) {
      // Time to scale down!
      scale_chmap(chmap, false);
    }
  }

  pthread_rwlock_unlock(&chmap->lock);
}

uint32_t chmap_elem_count(chashmap* chmap) {
  uint32_t r = 0;

  if (chmap) {
    pthread_rwlock_rdlock(&chmap->lock);
    r = chmap->elem_count;
    pthread_rwlock_unlock(&chmap->lock);
  }

  return r;
}

void chmap_for_each_elem_wr(chashmap* chmap,
                            void (*callback)(const chmap_pair* key_pair,
                                             chmap_pair* val_pair, void* args),
                            void* args) {
  if (!chmap || !callback) {
    return;
  }

  pthread_rwlock_wrlock(&chmap->lock);

  dllist_ref_node* tracker = chmap->head_of_all_elems;
  while (tracker) {
    (*callback)(tracker->host->data.key_pair, tracker->host->data.val_pair,
                args);
    tracker = tracker->next;
  }

  pthread_rwlock_unlock(&chmap->lock);
}

void chmap_for_each_elem_rd(chashmap* chmap,
                            void (*callback)(const chmap_pair* key_pair,
                                             const chmap_pair* val_pair,
                                             void* args),
                            void* args) {
  if (!chmap || !callback) {
    return;
  }

  pthread_rwlock_wrlock(&chmap->lock);

  dllist_ref_node* tracker = chmap->head_of_all_elems;
  while (tracker) {
    (*callback)(tracker->host->data.key_pair, tracker->host->data.val_pair,
                args);
    tracker = tracker->next;
  }

  pthread_rwlock_unlock(&chmap->lock);
}

int chmap_reset(chashmap* chmap, uint32_t new_bucket_array_size) {
  if (!chmap) {
    return -1;
  }

  int success = 0;

  if (new_bucket_array_size > 0 &&
      new_bucket_array_size < minimum_allowed_bucket_array_size) {
    new_bucket_array_size = minimum_allowed_bucket_array_size;
  } else {
    new_bucket_array_size = powl(2, ceill(log2l(new_bucket_array_size)));
  }

  pthread_rwlock_wrlock(&chmap->lock);

  for (uint32_t i = 0; i < chmap->bucket_arr_size; ++i) {
    destroy_the_whole_llist(chmap->bucket_arr[i], &chmap->head_of_all_elems);
  }

  if (new_bucket_array_size > 0 &&
      new_bucket_array_size != chmap->bucket_arr_size) {
    llist_node** orig = chmap->bucket_arr;
    chmap->bucket_arr =
        realloc(chmap->bucket_arr, new_bucket_array_size * sizeof(llist_node*));
    if (!chmap->bucket_arr) {
      chmap->bucket_arr = orig;
      success = -1;
    } else {
      chmap->bucket_arr_size = new_bucket_array_size;
    }
  }

  memset(chmap->bucket_arr, 0, chmap->bucket_arr_size * sizeof(llist_node*));
  chmap->elem_count = 0;

  pthread_rwlock_unlock(&chmap->lock);

  return success;
}

void _chmap_destroy(chashmap* chmap) {
  if (chmap) {
    pthread_rwlock_destroy(&chmap->lock);
    for (uint32_t i = 0; i < chmap->bucket_arr_size; ++i) {
      destroy_the_whole_llist(chmap->bucket_arr[i], &chmap->head_of_all_elems);
    }
    free((void*)chmap->bucket_arr);
    free(chmap);
  }
}
