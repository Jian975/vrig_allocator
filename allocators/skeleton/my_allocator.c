#include "allocator.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define HEAP_SIZE 1000000000
#define METADATA_SIZE 9900000

//allocate memory
static char heap[HEAP_SIZE];
static node_t metadata[METADATA_SIZE];
static int32_t metadata_size = 1;

static int delta(int a, int b) {
    int difference = a - b;
    if (difference < 0) {
        return -difference;
    }
    return difference;
}

static int minimum(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

//shift all values to the right of i (excluding i) by 1 to the right
static void shift_right(int i) {
    for (int j = metadata_size; j > i + 1; j--) {
        metadata[j] = metadata[j - 1];
    }
}

static int find(void * address) {
    for (int i = 0; i < metadata_size; i++) {
        if (metadata[i].address == address) {
            return i;
        }
    }
    return -1;
}
//shift all values to the right of i (excluding i) by 1
static void shift_left(int i) {
    for (int j = i + 1; j < metadata_size - 1; j++) {
        metadata[j] = metadata[j + 1];
    }
}

void print_memory() {
    for (int i = 0; i < metadata_size; i++) {
        ptrdiff_t relative_address = (uintptr_t) metadata[i].address - (uintptr_t) heap;
        printf("[size=%d, allocated=%d, address=%p]\n", 
            metadata[i].size, metadata[i].allocated, relative_address);
    }
    printf("\n");
}

/*
 * Replace these stubs with your own logic.
 */

static int my_init(void) {
    printf("heap address: %p\n", heap);
    metadata[0].allocated = 0;
    metadata[0].size = HEAP_SIZE;
    metadata[0].address = heap;
    metadata_size = 1;
    return 0;
}

static void my_teardown(void) {}

static void * my_malloc(int size) {
    //best fit
    int8_t current = 0;
    int smallest_delta = -1;
    int8_t best_fit = -1;
    while (current < metadata_size) {
        int current_delta = delta(metadata[current].size, size);
        if (metadata[current].allocated == 0 &&
            metadata[current].size >= size && 
            (smallest_delta == -1 || current_delta < smallest_delta)) {
            smallest_delta = current_delta;
            best_fit = current;
        }
        current += 1;
    }
    //out of memory
    if (best_fit == -1) {
        return NULL;
    }
    //split memory block if necessary
    if (metadata[best_fit].size > size) {
        shift_right(best_fit);
        metadata_size++;
        node_t * new_node = &metadata[best_fit + 1];
        new_node -> allocated = 0;
        new_node -> address = (char *) metadata[best_fit].address + size;
        new_node -> size = delta(metadata[best_fit].size, size);
        metadata[best_fit].size = size;
    }
    metadata[best_fit].allocated = 1;
    return metadata[best_fit].address;
}

static void my_free(void * address) {
    if (address == NULL) {
        return;
    }
    if (metadata_size == 1) {
        my_init();
        return;
    }
    int8_t freed = find(address);
    if (freed == -1) {
        printf("Error: Can't find address to free\n");
        return;
    }
    metadata[freed].allocated = 0;
    //if the block after this is free, coalesce
    if (freed < metadata_size - 1 && metadata[freed + 1].allocated == 0) {
        metadata[freed].size += metadata[freed + 1].size;
        shift_left(freed);
        metadata_size--;
    }
    //if the block before this is free, coalesce
    if (freed > 0 && metadata[freed - 1].allocated == 0) {
        metadata[freed - 1].size += metadata[freed].size;
        shift_left(freed - 1);
        metadata_size--;
    }
}

static void *my_realloc(void *ptr, size_t size) {
  if (size == 0) {
	  my_free(ptr);
	  return NULL;
  }
  int original = find(ptr);
  printf("old position:%d\n", original);
  printf("old size: %d\n", metadata[original].size);
  if (metadata[original].size == size) {
	  return ptr;
  }
  char * address = my_malloc(size);
  if (ptr == NULL) {
	  return address;
  }
  if (address == NULL) {
    	return NULL;
  }
  char * address_start = address;
  char * char_ptr = (char *) ptr;
  for (int i = 0; i < minimum(metadata[original].size, size); i++) {
    *address = *char_ptr;
    address++;
    char_ptr++;
  }
  while (address < address_start + size) {
	  *address = '\0';
	  address++;
  }
  my_free(ptr);
  return address_start;
}

static void *my_calloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  if (total < nmemb || size < size) {
	  return NULL;//overflow
  }
  char * ptr = my_malloc(total);
  char * ptr_start = ptr;
  for (size_t i = 0; i < total; i++) {
    *ptr = '\0';
    ptr++;
  }
  return ptr_start;
}

allocator_t allocator = {.malloc = my_malloc,
                         .free = my_free,
                         .realloc = my_realloc,
                         .calloc = my_calloc,
                         .init = my_init,
                         .teardown = my_teardown,
                         .name = "xuejiansundvall",
                         .author = "Xuejian Sundvall",
                         .version = "0.1.0",
                         .description = "My first custom allocator",
                         .memory_backend = "none",
                         .features = {.thread_safe = false,
                                      .per_thread_cache = false,
                                      .huge_page_support = false,
                                      .guard_pages = false,
                                      .guard_location = GUARD_NONE,
                                      .min_alignment = 8,
                                      .max_alignment = 4096}};

allocator_t *get_test_allocator(void) { return &allocator; }

allocator_t *get_bench_allocator(void) { return &allocator; }
