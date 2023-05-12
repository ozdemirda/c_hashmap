#include <chashmap.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tau/tau.h>
TAU_MAIN()  // sets up Tau (+ main function)

// HASH_MAP TESTS

TEST(chash_maps, create_fails) {
  chashmap* chmap = chmap_create(0);
  REQUIRE_EQ((void*)chmap, NULL);
}

TEST(chash_maps, create_succeeds) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  chmap_destroy(chmap);
  REQUIRE_EQ((void*)chmap, NULL);
}

// Helper functions start.
int insert_string_to_int(chashmap* chmap, const char* key, int val) {
  return chmap_insert_elem(
      chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key) + 1},
      &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)});
}

int get_int_from_string(chashmap* chmap, const char* key, int* val_ptr) {
  return chmap_get_elem_copy(
      chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key) + 1}, val_ptr,
      sizeof(int));
}

int get_int_ref_from_string(chashmap* chmap, const char* key, int** val_ptr) {
  chmap_pair* tmp_val_pair_ptr = NULL;
  if (chmap_get_elem_ref(
          chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key) + 1},
          &tmp_val_pair_ptr) == 0) {
    *val_ptr = (int*)tmp_val_pair_ptr->ptr;
    return 0;
  }
  return -1;
}

void delete_int_from_string(chashmap* chmap, const char* key) {
  chmap_delete_elem(chmap,
                    &(chmap_pair){.ptr = (void*)key, .size = strlen(key) + 1});
}
// Helper functions end.

TEST(chash_maps, basic_insertions_and_lookups) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  int val = -1;

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), -1);
  REQUIRE_EQ(get_int_from_string(chmap, "", &val), -1);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 2), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);
  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), 0);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), 0);
  REQUIRE_EQ(val, 5);

  chmap_destroy(chmap);
}

TEST(chash_maps, insert_values_with_different_sizes) {
  chashmap* chmap = chmap_create(1);

  const char* key1 = "key";

  struct s1 {
    unsigned int m1;
    unsigned int m2;
  } val1;
  val1.m1 = 3;
  val1.m2 = 5;

  struct s2 {
    unsigned long m1;
    unsigned long m2;
    unsigned long m3;
    unsigned long m4;
  } val2;
  val2.m1 = 7;
  val2.m2 = 9;
  val2.m3 = 11;
  val2.m4 = 13;

  chmap_pair* target_pair = NULL;

  REQUIRE_EQ(
      chmap_insert_elem(
          chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1) + 1},
          &(chmap_pair){.ptr = (void*)&val1, .size = sizeof(val1)}),
      0);

  REQUIRE_EQ(
      chmap_get_elem_ref(
          chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1) + 1},
          &target_pair),
      0);

  REQUIRE_EQ(memcmp(target_pair->ptr, &val1, sizeof(val1)), 0);

  REQUIRE_EQ(
      chmap_insert_elem(
          chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1) + 1},
          &(chmap_pair){.ptr = (void*)&val2, .size = sizeof(val2)}),
      0);

  REQUIRE_EQ(
      chmap_get_elem_ref(
          chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1) + 1},
          &target_pair),
      0);

  REQUIRE_EQ(memcmp(target_pair->ptr, &val2, sizeof(val2)), 0);

  chmap_destroy(chmap);
}

TEST(chash_maps, insertions_with_a_variety_of_key_sizes) {
  chashmap* chmap = chmap_create(1);

  int val;
  int target = 0;
  char ch = 'A';

  val = 1;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &ch, .size = sizeof(ch)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      0);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &ch, .size = sizeof(ch)},
                          &target, sizeof(int)),
      0);
  REQUIRE_EQ(val, target);

  short sh = 32768;
  val = 2;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &sh, .size = sizeof(sh)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      0);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &sh, .size = sizeof(sh)},
                          &target, sizeof(int)),
      0);
  REQUIRE_EQ(val, target);

  int id = 0xabcdef01;
  val = 3;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &id, .size = sizeof(id)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      0);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &id, .size = sizeof(id)},
                          &target, sizeof(int)),
      0);
  REQUIRE_EQ(val, target);

  long l = 0xffeeddbbccaa1100;
  val = 4;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &l, .size = sizeof(l)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      0);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &l, .size = sizeof(l)},
                          &target, sizeof(int)),
      0);
  REQUIRE_EQ(val, target);

  char* a_string_key = "a string key for testing";
  for (uint32_t i = 0; i < 24; ++i) {
    char key[32] = {0};
    snprintf(key, 32, "%.*s", i, a_string_key);
    val = i + 5;

    REQUIRE_EQ(chmap_insert_elem(
                   chmap, &(chmap_pair){.ptr = key, .size = strlen(key) + 1},
                   &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
               0);
    REQUIRE_EQ(chmap_get_elem_copy(
                   chmap, &(chmap_pair){.ptr = key, .size = strlen(key) + 1},
                   &target, sizeof(int)),
               0);
    REQUIRE_EQ(val, target);
  }

  chmap_destroy(chmap);
}

TEST(chash_maps, basic_deletions) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  int val = -1;

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  delete_int_from_string(chmap, "key1");  // Should not have any side effects
  delete_int_from_string(chmap, "");      // Should not have any side effects

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), 0);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), 0);

  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), 0);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), 0);
  REQUIRE_EQ(val, 5);

  delete_int_from_string(chmap, "key1");
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), -1);

  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  delete_int_from_string(chmap, "key2");
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), -1);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  delete_int_from_string(chmap, "key1");  // Should not have any side effects
  delete_int_from_string(chmap, "");      // Should not have any side effects

  chmap_destroy(chmap);
}

TEST(chash_maps, accessing_references) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  int* val_ptr = NULL;

  REQUIRE_EQ(get_int_ref_from_string(chmap, "key1", &val_ptr), -1);
  REQUIRE_EQ((void*)val_ptr, NULL);
  REQUIRE_EQ(get_int_ref_from_string(chmap, "", &val_ptr), -1);
  REQUIRE_EQ((void*)val_ptr, NULL);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", -3), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", -5), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  int tmp;

  REQUIRE_EQ(get_int_ref_from_string(chmap, "key1", &val_ptr), 0);
  REQUIRE_NE((void*)val_ptr, NULL);
  REQUIRE_EQ(*val_ptr, -3);
  *val_ptr = 1;  // The changes made via pointer alters the map element directly
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &tmp), 0);
  REQUIRE_EQ(tmp, 1);

  REQUIRE_EQ(get_int_ref_from_string(chmap, "key2", &val_ptr), 0);
  REQUIRE_NE((void*)val_ptr, NULL);
  REQUIRE_EQ(*val_ptr, -5);
  *val_ptr = 7;  // The changes made via pointer alters the map element directly
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &tmp), 0);
  REQUIRE_EQ(tmp, 7);

  chmap_destroy(chmap);
}

TEST(chash_maps, reset) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(chmap_reset(chmap, 2), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 4), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 6), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  int tmp;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &tmp), 0);
  REQUIRE_EQ(tmp, 4);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &tmp), 0);
  REQUIRE_EQ(tmp, 6);

  // Reset with a different bucket size
  REQUIRE_EQ(chmap_reset(chmap, 8192), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 4), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 6), 0);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &tmp), 0);
  REQUIRE_EQ(tmp, 4);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &tmp), 0);
  REQUIRE_EQ(tmp, 6);

  chmap_destroy(chmap);
}

void square_elem(const chmap_pair* key_pair, chmap_pair* val_pair, void* args) {
  // This if block is there to make the compiler happy, it's meaningless.
  if (!key_pair) {
    *(int*)args = 0;
  }

  *(int*)val_pair->ptr *= *(int*)val_pair->ptr;
}

int exec_func_on_int_from_string(chashmap* chmap, const char* key,
                                 void (*callback)(const chmap_pair* key_pair,
                                                  chmap_pair* val_pair,
                                                  void* args),
                                 void* args) {
  return chmap_exec_func_on_elem(
      chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key) + 1},
      callback, args);
}

TEST(chash_maps, exec_func_on_elem) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), 0);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), 0);

  REQUIRE_EQ(exec_func_on_int_from_string(chmap, "key1", square_elem, NULL), 0);
  REQUIRE_EQ(exec_func_on_int_from_string(chmap, "key2", square_elem, NULL), 0);

  // Non-existent keys should not cause any problems
  REQUIRE_EQ(exec_func_on_int_from_string(chmap, "key", square_elem, NULL), -1);
  REQUIRE_EQ(exec_func_on_int_from_string(chmap, "", square_elem, NULL), -1);

  int val = -1;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), 0);
  REQUIRE_EQ(val, 9);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), 0);
  REQUIRE_EQ(val, 25);

  chmap_destroy(chmap);
}

void incr_elem(const chmap_pair* key_pair, chmap_pair* val_pair, void* args) {
  // This if block is there to make the compiler happy, it's meaningless.
  if (!key_pair) {
    *(int*)args = 0;
  }

  *(int*)val_pair->ptr += (unsigned long)args;
}

TEST(chash_maps, for_each_elem_wr) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), 0);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), 0);

  // Add 7 to each one of the elements in the hash map
  chmap_for_each_elem_wr(chmap, incr_elem, (void*)7);

  int val = -1;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), 0);
  REQUIRE_EQ(val, 10);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), 0);
  REQUIRE_EQ(val, 12);

  chmap_destroy(chmap);
}

void add_elem_to_sum(const chmap_pair* key_pair, const chmap_pair* val_pair,
                     void* args) {
  // This if block is there to make the compiler happy, it's meaningless.
  if (!key_pair) {
    *(int*)args = 0;
  }

  int* sum = (int*)args;
  *sum += *(int*)val_pair->ptr;
}

TEST(chash_maps, for_each_elem_rd) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), 0);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), 0);

  int sum = 0;

  // Add 3 and 5 on top of sum. It should be 8
  // when the following statement is completed.
  chmap_for_each_elem_rd(chmap, add_elem_to_sum, (void*)&sum);

  REQUIRE_EQ(sum, 8);

  int val = -1;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), 0);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), 0);
  REQUIRE_EQ(val, 5);

  chmap_destroy(chmap);
}

extern uint32_t chmap_get_bucket_arr_size(chashmap* chmap);
extern uint32_t chmap_get_elem_count_to_scale_up(chashmap* chmap);
extern uint32_t chmap_get_elem_count_to_scale_down(chashmap* chmap);

TEST(chash_maps, scaling) {
  chashmap* chmap = chmap_create(1);
  REQUIRE_NE((void*)chmap, NULL);

  uint32_t first_up_threshold = chmap_get_elem_count_to_scale_up(chmap);
  uint32_t first_down_threshold = chmap_get_elem_count_to_scale_down(chmap);
  uint32_t first_capacity = chmap_get_bucket_arr_size(chmap);

  char key_buf[16] = {0};
  for (uint32_t i = 0; i <= first_up_threshold; ++i) {
    snprintf(key_buf, 16, "key%u", i);
    REQUIRE_EQ(insert_string_to_int(chmap, key_buf, 3), 0);
  }

  REQUIRE_EQ(chmap_elem_count(chmap), first_up_threshold + 1);

  REQUIRE_TRUE(first_up_threshold < chmap_get_elem_count_to_scale_up(chmap));
  REQUIRE_TRUE(first_down_threshold <
               chmap_get_elem_count_to_scale_down(chmap));
  REQUIRE_TRUE(first_capacity < chmap_get_bucket_arr_size(chmap));

  for (uint32_t i = 0; i <= first_up_threshold; ++i) {
    snprintf(key_buf, 16, "key%u", i);
    delete_int_from_string(chmap, key_buf);
  }

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_TRUE(first_up_threshold == chmap_get_elem_count_to_scale_up(chmap));
  REQUIRE_TRUE(first_down_threshold ==
               chmap_get_elem_count_to_scale_down(chmap));
  REQUIRE_TRUE(first_capacity == chmap_get_bucket_arr_size(chmap));

  chmap_destroy(chmap);
}
