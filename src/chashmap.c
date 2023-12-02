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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define mem_alloc(size) malloc(size)
#define mem_calloc(elem_count, elem_size) calloc(elem_count, elem_size)
#define mem_realloc(ptr, new_size) realloc(ptr, new_size)
#define mem_free(ptr) free(ptr)

#define _mem_alloc(m_procs, size) \
  (m_procs) ? m_procs->malloc(size) : mem_alloc(size)
#define _mem_calloc(m_procs, e_count, e_size) \
  (m_procs) ? m_procs->calloc(e_count, e_size) : mem_calloc(e_count, e_size)
#define _mem_realloc(m_procs, ptr, new_size) \
  (m_procs) ? m_procs->realloc(ptr, new_size) : mem_realloc(ptr, new_size)
#define _mem_free(m_procs, ptr) (m_procs) ? m_procs->free(ptr) : mem_free(ptr)

#define stringify(s) #s
#define x_stringify(s) stringify(s)
#define CERR_STR(x) (__FILE__ ":" x_stringify(__LINE__) " - " x)

const uint32_t minimum_allowed_bucket_array_size = 64;
const uint32_t scale_factor = 4;
const uint32_t minimum_scale_down_threshold =
    scale_factor * minimum_allowed_bucket_array_size;

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

typedef struct chmap_entry {
  unsigned long hash_val;
  chmap_pair* key_pair;
  chmap_pair* val_pair;
  chashmap_memmgmt_procs_t* m_procs;
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
  chashmap_memmgmt_procs_t* m_procs;
};

void destroy_llist_node(dllist_ref_node** head_of_all_elems, llist_node* elem) {
  if (elem) {
    if (elem->m_procs) {
      void (*free_func)(void*) = elem->m_procs->free;
      if (elem->data.key_pair->ptr) free_func(elem->data.key_pair->ptr);
      if (elem->data.key_pair) free_func(elem->data.key_pair);
      if (elem->data.val_pair->ptr) free_func(elem->data.val_pair->ptr);
      if (elem->data.val_pair) free_func(elem->data.val_pair);
      if (head_of_all_elems) {
        detach_node_from_dllist(head_of_all_elems, &elem->dllist_refs);
      }
      free_func(elem);
    } else {
      if (elem->data.key_pair->ptr) mem_free(elem->data.key_pair->ptr);
      if (elem->data.key_pair) mem_free(elem->data.key_pair);
      if (elem->data.val_pair->ptr) mem_free(elem->data.val_pair->ptr);
      if (elem->data.val_pair) mem_free(elem->data.val_pair);
      if (head_of_all_elems) {
        detach_node_from_dllist(head_of_all_elems, &elem->dllist_refs);
      }
      mem_free(elem);
    }
  }
}

llist_node* create_llist_node(dllist_ref_node** head_of_all_elems,
                              chmap_entry* data) {
  llist_node* new_elem =
      (llist_node*)_mem_calloc(data->m_procs, 1, sizeof(llist_node));
  if (!new_elem) {
    return NULL;
  }
  new_elem->m_procs = data->m_procs;

  new_elem->data.key_pair =
      (chmap_pair*)_mem_alloc(data->m_procs, sizeof(chmap_pair));
  if (!new_elem->data.key_pair) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  new_elem->data.val_pair =
      (chmap_pair*)_mem_alloc(data->m_procs, sizeof(chmap_pair));
  if (!new_elem->data.val_pair) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  new_elem->data.key_pair->ptr =
      _mem_alloc(data->m_procs, data->key_pair->size);
  if (!new_elem->data.key_pair->ptr) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  new_elem->data.val_pair->ptr =
      _mem_alloc(data->m_procs, data->val_pair->size);
  if (!new_elem->data.val_pair->ptr) {
    destroy_llist_node(head_of_all_elems, new_elem);
    return NULL;
  }

  attach_node_to_dllist(head_of_all_elems, &new_elem->dllist_refs, new_elem);
  new_elem->next = NULL;
  new_elem->data.hash_val = data->hash_val;
  new_elem->data.key_pair->size = data->key_pair->size;
  new_elem->data.val_pair->size = data->val_pair->size;
  mem_assign(new_elem->data.key_pair->ptr, data->key_pair->ptr,
             data->key_pair->size);
  mem_assign(new_elem->data.val_pair->ptr, data->val_pair->ptr,
             data->val_pair->size);

  return new_elem;
}

bool reset_val_of_llist_node(llist_node* elem, const chmap_pair* val_pair) {
  bool success = false;

  if (elem->data.val_pair->size == val_pair->size) {
    // The new value has the same size
    mem_assign(elem->data.val_pair->ptr, val_pair->ptr, val_pair->size);
    success = true;
  } else {
    // Sizes do not match, trying to reallocate.
    void* orig_buf = elem->data.val_pair->ptr;

    elem->data.val_pair->ptr =
        _mem_realloc(elem->m_procs, elem->data.val_pair->ptr, val_pair->size);
    if (!elem->data.val_pair->ptr) {
      // Failed to reallocate.
      elem->data.val_pair->ptr = orig_buf;
    } else {
      // Buffer reallocated, all is good.
      mem_assign(elem->data.val_pair->ptr, val_pair->ptr, val_pair->size);
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
  uint32_t elem_count;
  uint32_t bucket_arr_size;
  uint32_t elem_count_to_scale_up;
  uint32_t elem_count_to_scale_down;
  llist_node** bucket_arr;
  dllist_ref_node* head_of_all_elems;
  chashmap_memmgmt_procs_t* m_procs;
};

void set_chmap_scaling_limits(chashmap* chmap) {
  chmap->elem_count_to_scale_up = chmap->bucket_arr_size * 6 / 4;
  chmap->elem_count_to_scale_down = chmap->bucket_arr_size / 8;
}

#define POWERS_OF_TWO_LEN 32
uint32_t uint32_powers_of_two[POWERS_OF_TWO_LEN] =
    {
        1,          2,         4,        8,         16,        32,
        64,         128,       256,      512,       1024,      2048,
        4096,       8192,      16384,    32768,     65536,     131072,
        262144,     524288,    1048576,  2097152,   4194304,   8388608,
        16777216,   33554432,  67108864, 134217728, 268435456, 536870912,
        1073741824, 2147483648};  // 4294967296 exceeds 32 bits

// Kind of a "pow(2, ceil(log2(input)))" without the math library.
uint32_t find_nearest_gte_power_of_two(uint32_t input) {
  int left = 0;
  int right = POWERS_OF_TWO_LEN - 1;
  int middle = (left + right) / 2;

  if (input <= uint32_powers_of_two[0]) {
    return uint32_powers_of_two[0];
  }

  if (input >= uint32_powers_of_two[POWERS_OF_TWO_LEN - 1]) {
    return uint32_powers_of_two[POWERS_OF_TWO_LEN - 1];
  }

  uint32_t result = 0;

  while (left < right) {
    if (uint32_powers_of_two[middle] == input ||
        (middle > 0 && uint32_powers_of_two[middle - 1] < input &&
         input < uint32_powers_of_two[middle])) {
      result = uint32_powers_of_two[middle];
      break;
    } else if (middle < (POWERS_OF_TWO_LEN - 1) &&
               uint32_powers_of_two[middle] < input &&
               input <= uint32_powers_of_two[middle + 1]) {
      result = uint32_powers_of_two[middle + 1];
      break;
    }

    if (input < uint32_powers_of_two[middle]) {
      right = middle;
    } else {
      left = middle;
    }

    middle = (left + right) / 2;
  }

  return result;
}

bool verify_chmap_create_inputs(uint32_t initial_bucket_array_size,
                                chashmap_memmgmt_procs_t* mmgmt_procs,
                                char** err) {
  if (initial_bucket_array_size == 0) {
    if (err) {
      *err = CERR_STR("The initial bucket size is zero");
    }
    return false;
  }

  if (mmgmt_procs && (!mmgmt_procs->malloc || !mmgmt_procs->calloc ||
                      !mmgmt_procs->realloc || !mmgmt_procs->free)) {
    if (err) {
      *err = CERR_STR("Detected at least one NULL memory management function");
    }
    return false;
  }

  return true;
}

bool populate_mem_mgmt_procs(chashmap* chmap,
                             chashmap_memmgmt_procs_t* mmgmt_procs,
                             char** err) {
  if (mmgmt_procs) {
    chmap->m_procs = mmgmt_procs->malloc(sizeof(chashmap_memmgmt_procs_t));
    if (!chmap->m_procs) {
      if (err) {
        *err = CERR_STR("Failed to allocate buffer for memory mgmt buffer");
      }
      mmgmt_procs->free(chmap);
      return false;
    }
    memcpy(chmap->m_procs, mmgmt_procs, sizeof(chashmap_memmgmt_procs_t));
  } else {
    chmap->m_procs = NULL;
  }

  return true;
}

chashmap* chmap_create_mp(uint32_t initial_bucket_array_size,
                          chashmap_memmgmt_procs_t* mmgmt_procs, char** err) {
  if (!verify_chmap_create_inputs(initial_bucket_array_size, mmgmt_procs,
                                  err)) {
    return NULL;
  }

  if (initial_bucket_array_size <= minimum_allowed_bucket_array_size) {
    initial_bucket_array_size = minimum_allowed_bucket_array_size;
  } else {
    initial_bucket_array_size =
        find_nearest_gte_power_of_two(initial_bucket_array_size);
  }

  chashmap* chmap = (chashmap*)_mem_alloc(mmgmt_procs, sizeof(chashmap));
  if (!chmap) {
    if (err) {
      *err = CERR_STR("Failed to allocate buffer");
    }
    return NULL;
  }

  if (!populate_mem_mgmt_procs(chmap, mmgmt_procs, err)) {
    return NULL;
  }

  chmap->bucket_arr_size = initial_bucket_array_size;
  chmap->elem_count = 0;
  chmap->head_of_all_elems = NULL;
  set_chmap_scaling_limits(chmap);

  chmap->bucket_arr = (llist_node**)_mem_calloc(
      mmgmt_procs, initial_bucket_array_size, sizeof(llist_node*));
  if (!chmap->bucket_arr) {
    // Failed to allocate buffer for bucket_arr
    if (err) {
      *err = CERR_STR("Failed to allocate bucket_arr");
    }
    _mem_free(mmgmt_procs, chmap->m_procs);
    _mem_free(mmgmt_procs, chmap);
    return NULL;
  }

  if (err) {
    *err = NULL;
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
    // DJB2
    id = 5381;
    for (uint32_t i = 0; i < size; ++i) {
      id = ((id << 5) + id) + c_key_ptr[i];
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

  return chmap->bucket_arr_size;
}

uint32_t chmap_get_elem_count_to_scale_up(chashmap* chmap) {
  if (!chmap) {
    return 0;
  }

  return chmap->elem_count_to_scale_up;
}

uint32_t chmap_get_elem_count_to_scale_down(chashmap* chmap) {
  if (!chmap) {
    return 0;
  }

  return chmap->elem_count_to_scale_down;
}
#endif

void scale_chmap(chashmap* chmap, bool up) {
  uint32_t new_bucket_array_size = 0;
  if (up) {
    new_bucket_array_size = chmap->bucket_arr_size * scale_factor;
  } else {
    new_bucket_array_size = chmap->bucket_arr_size / scale_factor;
  }

  llist_node** new_bucket_arr;
  new_bucket_arr = (llist_node**)_mem_calloc(
      chmap->m_procs, new_bucket_array_size, sizeof(llist_node*));
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

  _mem_free(chmap->m_procs, chmap->bucket_arr);

  chmap->bucket_arr = new_bucket_arr;
  chmap->bucket_arr_size = new_bucket_array_size;
  set_chmap_scaling_limits(chmap);
}

chashmap_retval_t chmap_insert_elem(chashmap* chmap, const chmap_pair* key_pair,
                                    const chmap_pair* val_pair) {
  if (!chmap || !key_pair || !val_pair || !key_pair->ptr || !val_pair->ptr ||
      key_pair->size == 0 || val_pair->size == 0) {
    return chm_invalid_arguments;
  }

  chmap_entry data = {.hash_val = 0,
                      .key_pair = (chmap_pair*)key_pair,
                      .val_pair = (chmap_pair*)val_pair,
                      .m_procs = chmap->m_procs};

  bool result = false;

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, &data.hash_val);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    // The entry already exists
    result = reset_val_of_llist_node(r, val_pair);
  } else {
    chmap->bucket_arr[index] = insert_into_llist(
        chmap->bucket_arr[index], &chmap->head_of_all_elems, &data, &result);
    if (result) {
      if (++chmap->elem_count >= chmap->elem_count_to_scale_up) {
        // Time to scale up!
        scale_chmap(chmap, true);
      }
    }
  }

  return result ? chm_success : chm_not_enough_memory;
}

chashmap_retval_t chmap_get_elem_copy(chashmap* chmap,
                                      const chmap_pair* key_pair,
                                      void* target_buf,
                                      uint32_t target_buf_size) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0 ||
      !target_buf || target_buf_size == 0) {
    return chm_invalid_arguments;
  }

  chashmap_retval_t result = chm_key_not_found;

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, NULL);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    uint32_t min_size = target_buf_size;
    if (r->data.val_pair->size < min_size) {
      min_size = r->data.val_pair->size;
    }
    mem_assign(target_buf, r->data.val_pair->ptr, min_size);
    result = chm_success;
  }

  return result;
}

chashmap_retval_t chmap_get_elem_ref(chashmap* chmap,
                                     const chmap_pair* key_pair,
                                     chmap_pair** val_pair) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0 ||
      !val_pair) {
    return chm_invalid_arguments;
  }

  chashmap_retval_t result = chm_key_not_found;

  uint32_t index =
      calculate_bucket_index(chmap->bucket_arr_size, key_pair, NULL);

  llist_node* r = find_in_llist(chmap->bucket_arr[index], key_pair);
  if (r) {
    *val_pair = r->data.val_pair;
    result = chm_success;
  }

  return result;
}

chashmap_retval_t chmap_delete_elem(chashmap* chmap,
                                    const chmap_pair* key_pair) {
  if (!chmap || !key_pair || !key_pair->ptr || key_pair->size == 0) {
    return chm_invalid_arguments;
  }

  bool found = false;

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
    return chm_success;
  }

  return chm_key_not_found;
}

uint32_t chmap_elem_count(chashmap* chmap) {
  if (chmap) {
    return chmap->elem_count;
  }

  return 0;
}

void chmap_for_each_elem(chashmap* chmap,
                         void (*callback)(const chmap_pair* key_pair,
                                          chmap_pair* val_pair, void* args),
                         void* args) {
  if (!chmap || !callback) {
    return;
  }

  dllist_ref_node* tracker = chmap->head_of_all_elems;
  while (tracker) {
    (*callback)(tracker->host->data.key_pair, tracker->host->data.val_pair,
                args);
    tracker = tracker->next;
  }
}

chashmap_retval_t chmap_reset(chashmap* chmap, uint32_t new_bucket_array_size) {
  if (!chmap) {
    return chm_invalid_arguments;
  }

  int result = chm_success;

  if (new_bucket_array_size > 0 &&
      new_bucket_array_size < minimum_allowed_bucket_array_size) {
    new_bucket_array_size = minimum_allowed_bucket_array_size;
  } else {
    new_bucket_array_size =
        find_nearest_gte_power_of_two(new_bucket_array_size);
  }

  for (uint32_t i = 0; i < chmap->bucket_arr_size; ++i) {
    destroy_the_whole_llist(chmap->bucket_arr[i], &chmap->head_of_all_elems);
  }

  if (new_bucket_array_size > 0 &&
      new_bucket_array_size != chmap->bucket_arr_size) {
    llist_node** orig = chmap->bucket_arr;

    chmap->bucket_arr =
        _mem_realloc(chmap->m_procs, chmap->bucket_arr,
                     new_bucket_array_size * sizeof(llist_node*));
    if (!chmap->bucket_arr) {
      chmap->bucket_arr = orig;
      result = chm_not_enough_memory;
    } else {
      chmap->bucket_arr_size = new_bucket_array_size;
    }
  }

  memset(chmap->bucket_arr, 0, chmap->bucket_arr_size * sizeof(llist_node*));
  chmap->elem_count = 0;

  return result;
}

void __chmap_destroy(chashmap* chmap) {
  if (chmap) {
    for (uint32_t i = 0; i < chmap->bucket_arr_size; ++i) {
      destroy_the_whole_llist(chmap->bucket_arr[i], &chmap->head_of_all_elems);
    }
    if (chmap->m_procs) {
      void (*free_func)(void*) = chmap->m_procs->free;
      free_func((void*)chmap->bucket_arr);
      free_func(chmap->m_procs);
      free_func(chmap);
    } else {
      mem_free((void*)chmap->bucket_arr);
      mem_free(chmap);
    }
  }
}
