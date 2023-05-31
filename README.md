A minimalist, but generic hashmap implementation in plain C. Although the
library has some optimizations for integral data types, it can still be used
for storing any key-value pairs that contain references to valid memory areas.

A `chmap_pair` struct has the following layout:
```c
struct chmap_pair {
  void* ptr;
  uint32_t size;
}
```

Here, `ptr` is meant to be used to hold the address of a key or a value, while
`size` is there to keep the size of that memory area.

Let's have a look at a basic example to create a hashmap, and insert an element
into it using the UGLY approach:

```c
#include <chashmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // The value 256 is a hint for the initial hash bucket size
    // The bucket size is managed by the hashmap itself.
    chashmap* chmap = chmap_create(256);

    chmap_insert_elem(chmap,
        &(chmap_pair){.ptr = "key1", .size = strlen("key1")},
        &(chmap_pair){.ptr = &(int){3}, .size = sizeof(int)});

    int val;
    chmap_get_elem_copy(chmap,
        &(chmap_pair){.ptr = "key1", .size = strlen("key1")},
        &val, sizeof(val));

    printf("val: %d\n", val);

    chmap_destroy(chmap);
}
```

Now a hopefully more maintainable way of doing something similar:

```c
#include <chashmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A wrapper function: insert integer keyed by string
int insert_int_kb_string(chashmap* chmap, const char *key, int val) {
    return chmap_insert_elem(chmap,
        &(chmap_pair){.ptr = key, .size = strlen(key)},
        &(chmap_pair){.ptr = &val, .size = sizeof(int)});
}

// Another wrapper function
int get_int_kb_string(chashmap* chmap, const char *key, int *dest) {
    return chmap_get_elem_copy(chmap,
        &(chmap_pair){.ptr = key, .size = strlen(key)},
        dest, sizeof(int));
}

// The final wrapper function
void delete_int_kb_string(chashmap* chmap, char *key) {
    chmap_delete_elem(chmap,
        &(chmap_pair){.ptr = key, .size = strlen(key)});
}

int main() {
    // The value 256 is a hint for the initial hash bucket size
    // The bucket size is managed by the hashmap itself.
    chashmap* chmap = chmap_create(256);

    insert_int_kb_string(chmap, "key1", 3);

    int val;
    get_int_kb_string(chmap, "key1", &val);

    delete_int_kb_string(chmap, "key1");

    printf("val: %d\n", val);

    chmap_destroy(chmap);

    return 0;
}
```

So, the recommendation is implementing your own wrappers based on your own use
case. Although I was tempted to implementing some wrappers at least for the
integral data types, I decided not to do it, as they would consume an additional
area in the program store, and for some use cases those would be a waste of
memory.

Please notice that `I did not check the return values of the functions in the`
`examples above not to ruin the readability, but you SHOULD check for them`. The
functions that return a `signed int` will return 0 on success, and -1 on
failure.

Here's a list of available functions/macros to give you and idea about the
supported operations:

```c
- chashmap* chmap_create(uint32_t initial_bucket_array_size);
- chmap_destroy(chmap) // `A macro`
- uint32_t chmap_elem_count(chashmap* chmap);
- int chmap_reset(chashmap* chmap, uint32_t new_bucket_array_size);
- int chmap_insert_elem(chashmap* chmap, const chmap_pair* key_pair,
                        const chmap_pair* val_pair);
- int chmap_get_elem_copy(chashmap* chmap, const chmap_pair* key_pair,
                          void* target_buf, uint32_t target_buf_size);
- int chmap_get_elem_ref(chashmap* chmap, const chmap_pair* key_pair,
                         chmap_pair** val_pair);
- void chmap_delete_elem(chashmap* chmap, const chmap_pair* key_pair);
- void chmap_for_each_elem(chashmap* chmap,
                           void (*callback)(const chmap_pair* key_pair,
                                            chmap_pair* val_pair, void* args),
                           void* args);
```
