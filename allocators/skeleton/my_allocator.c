#include "allocator.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define HEAP_SIZE 100000
#define METADATA_SIZE 99000

//allocate memory
static char * heap = NULL;
static node_t metadata[METADATA_SIZE];
static int32_t metadata_size = 1;
static int8_t free_list = -1;
//if free list is empty
static int is_empty = 0;

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
}

/*
 * Replace these stubs with your own logic.
 */

static int my_init(void) {
    if (heap == NULL) {
        heap = malloc(HEAP_SIZE);
    }
    metadata[0].allocated = 0;
    metadata[0].next_free = -1;
    metadata[0].size = HEAP_SIZE;
    metadata[0].address = heap;
    free_list = 0;
    return 0;
}

static void my_teardown(void) {}

static void *my_malloc(size_t size) {
    if (is_empty) {
        return NULL;
    }
    //best fit
    int8_t current = free_list;
    int8_t previous = -1;
    int smallest_delta = -1;
    int8_t best_fit = -1;
    int8_t before_best_fit = -1;//keep track for splitting memory chunks
    while (current != -1) {
        int current_delta = delta(metadata[current].size, size);
        if (metadata[current].size >= size && (smallest_delta == -1 || current_delta < smallest_delta)) {
            smallest_delta = current_delta;
            best_fit = current;
            before_best_fit = previous;
        }
        previous = current;
        current = metadata[current].next_free;
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
        new_node -> next_free = metadata[best_fit].next_free;
        new_node -> allocated = 0;
        new_node -> address = (char *) metadata[best_fit].address + size;
        new_node -> size = delta(metadata[best_fit].size, size);
        if (best_fit + 2 == metadata_size) {
            new_node -> next_free = -1;
        }
        if (before_best_fit != -1) {
            metadata[before_best_fit].next_free = best_fit + 1;
        } else{//no previous means we removed first node
            free_list = best_fit + 1;
        }
        metadata[best_fit].size = size;
    } else {
        if (before_best_fit != -1) {
            metadata[before_best_fit].next_free = metadata[best_fit].next_free;
            metadata[best_fit].size = size;
        } else {
            //We removed the only entry and we didn't split any memory chunks
            //free list is now empty
            is_empty = 1;
        }
    }

    metadata[best_fit].allocated = 1;
    print_memory();
    return metadata[best_fit].address;
}

static void my_free(void * address) {
    if (address == NULL) {
        return;
    }
    if (is_empty) {
        my_init();
    } else {
        int8_t last_free = -1;
        for (int i = 0; i < metadata_size; i++) {
            if (metadata[i].allocated == 0) {
                last_free = i;
            }
            if (metadata[i].address == address) {
                metadata[i].allocated = 0;
                if (i > 1 && metadata[i - 1].allocated == 0) {
                    metadata[i - 1].size += metadata[i].size;
                    metadata[i - 1].next_free = metadata[i].next_free;
                    shift_left(i - 1);
                    metadata_size--;
                    i--;
                }
                if (metadata[i + 1].allocated == 0) {
                    metadata[i].size += metadata[i + 1].size;
                    metadata[i].next_free = metadata[i + 1].next_free;
                    if (last_free == i + 1) {
                        metadata[last_free].next_free = i;
                    }
                    shift_left(i);
                    metadata_size--;
                    break;
                }
            }
        }
    }
    is_empty = 0;
    print_memory();
}

static void *my_realloc(void *ptr, size_t size) {
  char * address = my_malloc(size);
  if (address == NULL) {
    return NULL;
  }
  if (ptr == NULL) {
	  return address;
  }
  if (size == 0) {
	  my_free(ptr);
	  return NULL;
  }
  char * address_start = address;
  node_t * metadata = (node_t * ) ((char *) ptr - sizeof(node_t));
  char * char_ptr = (char *) ptr;
  for (int i = 0; i < minimum(metadata -> size, size); i++) {
    *address = *char_ptr;
    address++;
    char_ptr++;
  }
  my_free(ptr);
  return address_start;
}

static void *my_calloc(size_t nmemb, size_t size) {
  char * ptr = my_malloc(nmemb * size);
  for (size_t i = 0; i < (nmemb * size); i++) {
    *ptr = '\0';
    ptr++;
  }
  return ptr;
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
