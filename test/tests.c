#include <chashmap.h>
#include <stdlib.h>
#include <string.h>
#include <tau/tau.h>
TAU_MAIN()  // sets up Tau (+ main function)

// HASH_MAP TESTS

extern uint32_t find_nearest_gte_power_of_two(uint32_t input);

TEST(chash_maps, find_nearest_gte_power_of_two) {
  // Hand-crafted the following input and expected output arrays, as we don't
  // want to depend on a math lib to do the testing.

  uint32_t test_inputs[] = {
      0,          1,          2,          3,          4,          5,
      7,          8,          9,          15,         16,         17,
      31,         32,         33,         63,         64,         65,
      127,        128,        129,        255,        256,        257,
      511,        512,        513,        1023,       1024,       1025,
      2047,       2048,       2049,       4095,       4096,       4097,
      8191,       8192,       8193,       16383,      16384,      16385,
      32767,      32768,      32769,      65535,      65536,      65537,
      131071,     131072,     131073,     262143,     262144,     262145,
      524287,     524288,     524289,     1048575,    1048576,    1048577,
      2097151,    2097152,    2097153,    4194303,    4194304,    4194305,
      8388607,    8388608,    8388609,    16777215,   16777216,   16777217,
      33554431,   33554432,   33554433,   67108863,   67108864,   67108865,
      134217727,  134217728,  134217729,  268435455,  268435456,  268435457,
      536870911,  536870912,  536870913,  1073741823, 1073741824, 1073741825,
      2147483647, 2147483648, 2147483649, 4294967295};

  uint32_t expected_outputs[] = {
      1,          1,          2,          4,          4,          8,
      8,          8,          16,         16,         16,         32,
      32,         32,         64,         64,         64,         128,
      128,        128,        256,        256,        256,        512,
      512,        512,        1024,       1024,       1024,       2048,
      2048,       2048,       4096,       4096,       4096,       8192,
      8192,       8192,       16384,      16384,      16384,      32768,
      32768,      32768,      65536,      65536,      65536,      131072,
      131072,     131072,     262144,     262144,     262144,     524288,
      524288,     524288,     1048576,    1048576,    1048576,    2097152,
      2097152,    2097152,    4194304,    4194304,    4194304,    8388608,
      8388608,    8388608,    16777216,   16777216,   16777216,   33554432,
      33554432,   33554432,   67108864,   67108864,   67108864,   134217728,
      134217728,  134217728,  268435456,  268435456,  268435456,  536870912,
      536870912,  536870912,  1073741824, 1073741824, 1073741824, 2147483648,
      2147483648, 2147483648, 2147483648, 2147483648};

  int len = sizeof(test_inputs) / sizeof(uint32_t);
  REQUIRE_EQ(len, sizeof(expected_outputs) / sizeof(uint32_t));

  for (int i = 0; i < len; ++i) {
    uint32_t r = find_nearest_gte_power_of_two(test_inputs[i]);
    REQUIRE_EQ(r, expected_outputs[i]);
  }
}

TEST(chash_maps, create_fails) {
  char* err = NULL;
  chashmap* chmap = chmap_create(0, &err);
  REQUIRE_EQ((void*)chmap, NULL);
  REQUIRE_NE((void*)err, NULL);

  chmap = chmap_create_mp(
      4096,
      &(chashmap_memmgmt_procs_t){
          .malloc = NULL, .free = free, .calloc = calloc, .realloc = realloc},
      &err);
  REQUIRE_EQ((void*)chmap, NULL);
  REQUIRE_NE((void*)err, NULL);

  chmap = chmap_create_mp(
      4096,
      &(chashmap_memmgmt_procs_t){
          .malloc = malloc, .free = NULL, .calloc = calloc, .realloc = realloc},
      &err);
  REQUIRE_EQ((void*)chmap, NULL);
  REQUIRE_NE((void*)err, NULL);

  chmap = chmap_create_mp(
      4096,
      &(chashmap_memmgmt_procs_t){
          .malloc = malloc, .free = free, .calloc = NULL, .realloc = realloc},
      &err);
  REQUIRE_EQ((void*)chmap, NULL);
  REQUIRE_NE((void*)err, NULL);

  chmap = chmap_create_mp(
      4096,
      &(chashmap_memmgmt_procs_t){
          .malloc = malloc, .free = free, .calloc = calloc, .realloc = NULL},
      &err);
  REQUIRE_EQ((void*)chmap, NULL);
  REQUIRE_NE((void*)err, NULL);
}

TEST(chash_maps, create_succeeds) {
  char* err = "";
  chashmap* chmap = chmap_create(4096, &err);
  REQUIRE_NE((void*)chmap, NULL);
  REQUIRE_EQ((void*)err, NULL);

  chmap_destroy(chmap);
  REQUIRE_EQ((void*)chmap, NULL);

  chmap = chmap_create_mp(
      4096,
      &(chashmap_memmgmt_procs_t){
          .malloc = malloc, .free = free, .calloc = calloc, .realloc = realloc},
      &err);

  chmap_destroy(chmap);
  REQUIRE_EQ((void*)chmap, NULL);
}

// Helper functions start.
int insert_string_to_int(chashmap* chmap, const char* key, int val) {
  return chmap_insert_elem(
      chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key)},
      &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)});
}

int get_int_from_string(chashmap* chmap, const char* key, int* val_ptr) {
  return chmap_get_elem_copy(
      chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key)}, val_ptr,
      sizeof(int));
}

int get_int_ref_from_string(chashmap* chmap, const char* key, int** val_ptr) {
  chmap_pair* tmp_val_pair_ptr = NULL;
  if (chmap_get_elem_ref(chmap,
                         &(chmap_pair){.ptr = (void*)key, .size = strlen(key)},
                         &tmp_val_pair_ptr) == 0) {
    *val_ptr = (int*)tmp_val_pair_ptr->ptr;
    return 0;
  }
  return -1;
}

chashmap_retval_t delete_int_from_string(chashmap* chmap, const char* key) {
  return chmap_delete_elem(
      chmap, &(chmap_pair){.ptr = (void*)key, .size = strlen(key)});
}
// Helper functions end.

TEST(chash_maps, basic_insertions_and_lookups) {
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  int val = -1;

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_key_not_found);
  REQUIRE_EQ(get_int_from_string(chmap, "", &val), chm_invalid_arguments);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 2), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);
  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 4), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_success);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), chm_success);
  REQUIRE_EQ(val, 5);

  chmap_destroy(chmap);
}

TEST(chash_maps, basic_insertions_and_lookups_with_memmgmt_procs) {
  chashmap* chmap = chmap_create_mp(
      1,
      &(chashmap_memmgmt_procs_t){
          .malloc = malloc, .free = free, .calloc = calloc, .realloc = realloc},
      NULL);
  REQUIRE_NE((void*)chmap, NULL);

  int val = -1;

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_key_not_found);
  REQUIRE_EQ(get_int_from_string(chmap, "", &val), chm_invalid_arguments);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 2), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);
  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 4), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_success);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), chm_success);
  REQUIRE_EQ(val, 5);

  chmap_destroy(chmap);
}

TEST(chash_maps, insert_values_with_different_sizes) {
  chashmap* chmap = chmap_create(1, NULL);

  const char* key1 = "key";
  chmap_pair* target_pair = NULL;

  {
    struct s1 {
      unsigned int m1;
      unsigned int m2;
    } val, test_val;
    val.m1 = 3;
    val.m2 = 5;

    REQUIRE_EQ(
        chmap_insert_elem(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)}),
        chm_success);

    REQUIRE_EQ(
        chmap_get_elem_ref(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &target_pair),
        chm_success);

    REQUIRE_EQ(memcmp(target_pair->ptr, &val, sizeof(val)), 0);

    REQUIRE_EQ(
        chmap_get_elem_copy(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &test_val, sizeof(test_val)),
        chm_success);

    REQUIRE_EQ(memcmp(&test_val, &val, sizeof(val)), 0);
  }

  {
    struct s2 {
      unsigned long m1;
      unsigned long m2;
      unsigned long m3;
      unsigned long m4;
    } val, test_val;
    val.m1 = 7;
    val.m2 = 9;
    val.m3 = 11;
    val.m4 = 13;

    REQUIRE_EQ(
        chmap_insert_elem(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)}),
        chm_success);

    REQUIRE_EQ(
        chmap_get_elem_ref(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &target_pair),
        chm_success);

    REQUIRE_EQ(memcmp(target_pair->ptr, &val, sizeof(val)), 0);

    REQUIRE_EQ(
        chmap_get_elem_copy(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &test_val, sizeof(test_val)),
        chm_success);

    REQUIRE_EQ(memcmp(&test_val, &val, sizeof(val)), 0);
  }

  {
    unsigned long val = 0xabcdef01, test_val = 0xffffffff;

    REQUIRE_EQ(
        chmap_insert_elem(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)}),
        chm_success);

    REQUIRE_EQ(
        chmap_get_elem_ref(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &target_pair),
        chm_success);

    REQUIRE_EQ(memcmp(target_pair->ptr, &val, sizeof(val)), 0);

    REQUIRE_EQ(
        chmap_get_elem_copy(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &test_val, sizeof(test_val)),
        chm_success);

    REQUIRE_EQ(memcmp(&test_val, &val, sizeof(val)), 0);
  }

  {
    unsigned char val = 0xab, test_val = 0xcd;

    REQUIRE_EQ(
        chmap_insert_elem(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)}),
        chm_success);

    REQUIRE_EQ(
        chmap_get_elem_ref(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &target_pair),
        chm_success);

    REQUIRE_EQ(memcmp(target_pair->ptr, &val, sizeof(val)), 0);

    REQUIRE_EQ(
        chmap_get_elem_copy(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &test_val, sizeof(test_val)),
        chm_success);

    REQUIRE_EQ(memcmp(&test_val, &val, sizeof(val)), 0);
  }

  {
    unsigned short val = 0xabcd, test_val = 0xef01;

    REQUIRE_EQ(
        chmap_insert_elem(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &(chmap_pair){.ptr = (void*)&val, .size = sizeof(val)}),
        chm_success);

    REQUIRE_EQ(
        chmap_get_elem_ref(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &target_pair),
        chm_success);

    REQUIRE_EQ(memcmp(target_pair->ptr, &val, sizeof(val)), 0);

    REQUIRE_EQ(
        chmap_get_elem_copy(
            chmap, &(chmap_pair){.ptr = (void*)key1, .size = strlen(key1)},
            &test_val, sizeof(test_val)),
        chm_success);

    REQUIRE_EQ(memcmp(&test_val, &val, sizeof(val)), 0);
  }

  chmap_destroy(chmap);
}

TEST(chash_maps, insertions_with_a_variety_of_key_sizes) {
  chashmap* chmap = chmap_create(1, NULL);

  int val;
  int target = 0;
  char ch = 'A';

  val = 1;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &ch, .size = sizeof(ch)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      chm_success);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &ch, .size = sizeof(ch)},
                          &target, sizeof(int)),
      chm_success);
  REQUIRE_EQ(val, target);

  short sh = 32768;
  val = 2;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &sh, .size = sizeof(sh)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      chm_success);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &sh, .size = sizeof(sh)},
                          &target, sizeof(int)),
      chm_success);
  REQUIRE_EQ(val, target);

  int id = 0xabcdef01;
  val = 3;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &id, .size = sizeof(id)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      chm_success);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &id, .size = sizeof(id)},
                          &target, sizeof(int)),
      chm_success);
  REQUIRE_EQ(val, target);

  long l = 0xffeeddbbccaa1100;
  val = 4;
  REQUIRE_EQ(
      chmap_insert_elem(chmap, &(chmap_pair){.ptr = &l, .size = sizeof(l)},
                        &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
      chm_success);
  REQUIRE_EQ(
      chmap_get_elem_copy(chmap, &(chmap_pair){.ptr = &l, .size = sizeof(l)},
                          &target, sizeof(int)),
      chm_success);
  REQUIRE_EQ(val, target);

  char* a_string_key = "a string key for testing";
  for (uint32_t i = 1; i <= 24; ++i) {
    char key[32] = {0};
    snprintf(key, 32, "%.*s", i, a_string_key);
    val = i + 5;

    REQUIRE_EQ(
        chmap_insert_elem(chmap, &(chmap_pair){.ptr = key, .size = strlen(key)},
                          &(chmap_pair){.ptr = &val, .size = sizeof(val)}),
        chm_success);
    REQUIRE_EQ(chmap_get_elem_copy(
                   chmap, &(chmap_pair){.ptr = key, .size = strlen(key)},
                   &target, sizeof(int)),
               chm_success);
    REQUIRE_EQ(val, target);
  }

  chmap_destroy(chmap);
}

TEST(chash_maps, basic_deletions) {
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  int val = -1;

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(delete_int_from_string(chmap, "key1"),
             chm_key_not_found);  // Should not have any side effects
  REQUIRE_EQ(delete_int_from_string(chmap, ""),
             chm_invalid_arguments);  // Should not have any side effects

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), chm_success);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), chm_success);

  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_success);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), chm_success);
  REQUIRE_EQ(val, 5);

  REQUIRE_EQ(delete_int_from_string(chmap, "key1"), chm_success);
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_key_not_found);

  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(delete_int_from_string(chmap, "key2"), chm_success);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), chm_key_not_found);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(delete_int_from_string(chmap, "key1"),
             chm_key_not_found);  // Should not have any side effects
  REQUIRE_EQ(delete_int_from_string(chmap, ""),
             chm_invalid_arguments);  // Should not have any side effects

  chmap_destroy(chmap);
}

TEST(chash_maps, accessing_references) {
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  int* val_ptr = NULL;

  REQUIRE_EQ(get_int_ref_from_string(chmap, "key1", &val_ptr), -1);
  REQUIRE_EQ((void*)val_ptr, NULL);
  REQUIRE_EQ(get_int_ref_from_string(chmap, "", &val_ptr), -1);
  REQUIRE_EQ((void*)val_ptr, NULL);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", -3), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", -5), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  int tmp;

  REQUIRE_EQ(get_int_ref_from_string(chmap, "key1", &val_ptr), chm_success);
  REQUIRE_NE((void*)val_ptr, NULL);
  REQUIRE_EQ(*val_ptr, -3);
  *val_ptr = 1;  // The changes made via pointer alters the map element directly
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &tmp), chm_success);
  REQUIRE_EQ(tmp, 1);

  REQUIRE_EQ(get_int_ref_from_string(chmap, "key2", &val_ptr), chm_success);
  REQUIRE_NE((void*)val_ptr, NULL);
  REQUIRE_EQ(*val_ptr, -5);
  *val_ptr = 7;  // The changes made via pointer alters the map element directly
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &tmp), chm_success);
  REQUIRE_EQ(tmp, 7);

  chmap_destroy(chmap);
}

TEST(chash_maps, reset) {
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(chmap_reset(chmap, 2), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 4), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 6), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  int tmp;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &tmp), chm_success);
  REQUIRE_EQ(tmp, 4);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &tmp), chm_success);
  REQUIRE_EQ(tmp, 6);

  // Reset with a different bucket size
  REQUIRE_EQ(chmap_reset(chmap, 8192), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 0);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 4), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 1);

  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 6), chm_success);
  REQUIRE_EQ(chmap_elem_count(chmap), 2);

  REQUIRE_EQ(get_int_from_string(chmap, "key1", &tmp), chm_success);
  REQUIRE_EQ(tmp, 4);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &tmp), chm_success);
  REQUIRE_EQ(tmp, 6);

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
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), chm_success);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), chm_success);

  // Add 7 to each one of the elements in the hash map
  chmap_for_each_elem(chmap, incr_elem, (void*)7);

  int val = -1;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_success);
  REQUIRE_EQ(val, 10);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), chm_success);
  REQUIRE_EQ(val, 12);

  chmap_destroy(chmap);
}

void add_elem_to_sum(const chmap_pair* key_pair, chmap_pair* val_pair,
                     void* args) {
  // This if block is there to make the compiler happy, it's meaningless.
  if (!key_pair) {
    *(int*)args = 0;
  }

  int* sum = (int*)args;
  *sum += *(int*)val_pair->ptr;
}

TEST(chash_maps, for_each_elem_rd) {
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  REQUIRE_EQ(insert_string_to_int(chmap, "key1", 3), chm_success);
  REQUIRE_EQ(insert_string_to_int(chmap, "key2", 5), chm_success);

  int sum = 0;

  // Add 3 and 5 on top of sum. It should be 8
  // when the following statement is completed.
  chmap_for_each_elem(chmap, add_elem_to_sum, (void*)&sum);

  REQUIRE_EQ(sum, 8);

  int val = -1;
  REQUIRE_EQ(get_int_from_string(chmap, "key1", &val), chm_success);
  REQUIRE_EQ(val, 3);
  REQUIRE_EQ(get_int_from_string(chmap, "key2", &val), chm_success);
  REQUIRE_EQ(val, 5);

  chmap_destroy(chmap);
}

extern uint32_t chmap_get_bucket_arr_size(chashmap* chmap);
extern uint32_t chmap_get_elem_count_to_scale_up(chashmap* chmap);
extern uint32_t chmap_get_elem_count_to_scale_down(chashmap* chmap);

TEST(chash_maps, scaling) {
  chashmap* chmap = chmap_create(1, NULL);
  REQUIRE_NE((void*)chmap, NULL);

  uint32_t first_up_threshold = chmap_get_elem_count_to_scale_up(chmap);
  uint32_t first_down_threshold = chmap_get_elem_count_to_scale_down(chmap);
  uint32_t first_capacity = chmap_get_bucket_arr_size(chmap);

  char key_buf[16] = {0};
  for (uint32_t i = 0; i <= first_up_threshold; ++i) {
    snprintf(key_buf, 16, "key%u", i);
    REQUIRE_EQ(insert_string_to_int(chmap, key_buf, 3), chm_success);
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
