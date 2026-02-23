#include "allocator.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define HEAP_SIZE 502400000

//allocate memory
static char heap[HEAP_SIZE];
static node_t * free_list = NULL;
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

//given that first node goes before next node
//return 0 if the two nodes are not touching
static int can_merge(node_t * first, node_t * next) {
    return ((char*) (first + 1) + first -> size) == next;
}

//merges eaten into eater and returns pointer to eater
static node_t * merge(node_t * eater, node_t * eaten) {
    eater -> size += (eaten -> size + sizeof(node_t));
    eater -> next = eaten -> next;
    return eater;
}

/*
 * Replace these stubs with your own logic.
 */

static int my_init(void) {
    node_t * new_free_list = (node_t *) heap;
    new_free_list -> size = SIZE_MAX;
    new_free_list -> next = NULL;
    free_list = new_free_list;
    return 0;
}

static void my_teardown(void) {}

static void *my_malloc(size_t size) {
  if (is_empty) {
        return NULL;
    }
    //best fit
    node_t * current = free_list;
    node_t * previous = NULL;
    int smallest_delta = -1;
    node_t * best_fit = NULL;
    node_t * before_best_fit = NULL;//keep track for splitting memory chunks
    while (current != NULL) {
        int current_delta = delta(current -> size, size);
        if (current -> size >= size && (smallest_delta == -1 || current_delta < smallest_delta)) {
            smallest_delta = current_delta;
            best_fit = current;
            before_best_fit = previous;
        }
        previous = current;
        current = current -> next;
    }

    //out of memory
    if (best_fit == NULL) {
        return NULL;
    }

    //split memory block if necessary
    if (best_fit -> size > size + sizeof(node_t)) {
        node_t * new_node = (node_t *) (((char*) (best_fit + 1)) + size);
        new_node -> next = best_fit -> next;
        if (before_best_fit != NULL) {
            before_best_fit -> next = new_node;
        } else{//no previous means we removed first node
            free_list = new_node;
        }
        new_node -> size = delta(best_fit -> size, size + sizeof(node_t));
        best_fit -> size = (char*) new_node - (char*) best_fit - sizeof(node_t);
    } else {
        if (before_best_fit != NULL) {
            before_best_fit -> next = best_fit -> next;
            best_fit -> size = (char*) best_fit -> next - (char*) best_fit - sizeof(node_t);
        } else {
            //We removed the only entry and we didn't split any memory chunks
            //free list is now empty
            is_empty = 1;
        }
    }

    return best_fit + 1;
}

static void my_free(void *ptr) {
  if (ptr == NULL) {
        return;
    }
    if (is_empty) {
        my_init();
    } else {
        node_t * new_node = (node_t*) ptr - 1;
        node_t * current = free_list;
        node_t * previous = NULL;
        while (current != NULL) {
            if (current > new_node) {
                //freed block is first chunk in memory, with one or more following it
                if (previous == NULL) {
                    //merge freed block with next block if we can merge
                    if (can_merge(new_node, current)) {
                        new_node = merge(new_node, current);
                        free_list = new_node;
                    } else {
                        //can't merge, just insert into linked list
                        new_node -> next = current;
                        free_list = new_node;
                    }
                    
                } else {
                    //freed block is not first chunk in memory
                    previous -> next = new_node;
                    new_node -> next = current;
                    if (can_merge(previous, new_node)) {
                        new_node = merge(previous, new_node);
                    }
                    if (can_merge(new_node, current)) {
                        merge(new_node, current);
                    }
                }
                break;
            }
            previous = current;
            current = current -> next;
        }
    }
    is_empty = 0;
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
