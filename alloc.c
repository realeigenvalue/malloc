#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define METADATA_SIZE (sizeof(metadata_entry_t))
#define SPLIT_FACTOR (40)
#define MERGE_FACTOR (1)

typedef struct _metadata_entry_t {
    int size;
    int free_flag;
    struct _metadata_entry_t *next;
    struct _metadata_entry_t *prev;
} metadata_entry_t;

metadata_entry_t *metadata_head = NULL;
void *heap_start = NULL;

int is_block_free(metadata_entry_t *ptr);
void mark_block_free(metadata_entry_t *ptr);
void mark_block_used(metadata_entry_t *ptr);
size_t get_block_size(metadata_entry_t *ptr);
void *get_block_address(metadata_entry_t *ptr);
metadata_entry_t *get_block_metadata(void *ptr);
void set_block_size(metadata_entry_t *ptr, size_t size);
void set_block_footer(metadata_entry_t *ptr);
void split_block(metadata_entry_t *ptr, size_t trim_size);
metadata_entry_t *get_left_block(metadata_entry_t *ptr);
metadata_entry_t *get_right_block(metadata_entry_t *ptr);
void remove_block_node(metadata_entry_t *ptr);
void merge_block(metadata_entry_t *ptr);

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    size_t allocation_size = num * size;
    void *ptr = malloc(allocation_size);
    if(ptr == NULL) {
        return NULL;
    }
    memset(ptr, 0, allocation_size);
    return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t request) {
    // implement malloc!
    if(request == 0) {
        return NULL;
    }
    if(heap_start == NULL) {
        heap_start = sbrk(0);
    }
    request += (METADATA_SIZE - request) & (METADATA_SIZE - 1);
    metadata_entry_t *p = metadata_head;
    metadata_entry_t *chosen = NULL;
    while(p != NULL) {
        if(get_block_size(p) >= request) {
            chosen = p;
            remove_block_node(p);
            break;
        }
        p = p->next;
    }
    if(chosen != NULL) {
        mark_block_used(chosen);
        if(get_block_size(chosen) >= request * SPLIT_FACTOR) {
            split_block(chosen, request);
        }
        return get_block_address(chosen);
    }
    chosen = sbrk(0);
    if(sbrk(METADATA_SIZE + sizeof(int) + request) == (void *)-1) {
        return NULL;
    }
    set_block_size(chosen, request);
    mark_block_used(chosen);
    return get_block_address(chosen);
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    if(ptr == NULL) {
        return;
    }
    metadata_entry_t *p = get_block_metadata(ptr);
    merge_block(p);
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    void *newptr = NULL;
    size_t old_size = 0;
    if(ptr == NULL) {
        newptr = malloc(size); //return newptr
    } else if(size == 0) {
        free(ptr); //return NULL
    } else if((old_size = get_block_size(get_block_metadata(ptr)))>= size) {
        newptr = ptr; //return ptr
    } else {
        if((newptr = malloc(size)) != NULL) { //fails newptr = NULL
            memcpy(newptr, ptr, old_size);
            free(ptr);
        }
    }
    return newptr;
}

//HELPER FUNCTIONS

int is_block_free(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return 0;
    }
    return ptr->free_flag;
}

void mark_block_free(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return;
    }
    ptr->free_flag = 1;
}

void mark_block_used(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return;
    }
    ptr->free_flag = 0;
}

size_t get_block_size(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return 0;
    }
    return (size_t)ptr->size;
}

void *get_block_address(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return (void *)NULL;
    }
    return (void *)(ptr + 1);
}

metadata_entry_t *get_block_metadata(void *ptr) {
    if(ptr == NULL) {
        return NULL;
    }
    return (metadata_entry_t *)(ptr - METADATA_SIZE);
}

void set_block_size(metadata_entry_t *ptr, size_t size) {
    if(ptr == NULL) {
        return;
    }
    ptr->size = size;
    set_block_footer(ptr);
}

void set_block_footer(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return;
    }
    void *footer = (void *)ptr + METADATA_SIZE + get_block_size(ptr);
    if(footer < sbrk(0)) {
        *((int *)footer) = ptr->size;
    }
}

void split_block(metadata_entry_t *ptr, size_t trim_size) {
    if(ptr == NULL || trim_size == 0) {
        return;
    }
    metadata_entry_t *new_block = (metadata_entry_t *)((void *)(ptr + 1) + trim_size + sizeof(int));
    size_t new_block_size = get_block_size(ptr) - (trim_size + (METADATA_SIZE + sizeof(int)));
    set_block_size(ptr, trim_size);
    set_block_size(new_block, new_block_size);
    free(new_block + 1);
}

metadata_entry_t *get_left_block(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return NULL;
    }
    if((void *)(ptr - 1) < heap_start) {
        return NULL;
    }
    int left_block_size = *((int *)ptr - 1);
    metadata_entry_t *left_block = (metadata_entry_t *)((void *)ptr - (sizeof(int) + left_block_size + METADATA_SIZE));
    if(is_block_free(left_block)) {
        return left_block;
    }
    return NULL;
}

metadata_entry_t *get_right_block(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return NULL;
    }
    void *right_block_address = (void *)ptr + METADATA_SIZE + get_block_size(ptr) + sizeof(int);
    if(right_block_address + METADATA_SIZE >= sbrk(0)) {
        return NULL;
    }
    metadata_entry_t *right_block = (metadata_entry_t *)(right_block_address);
    if(is_block_free(right_block)) {
        return right_block;
    }
    return NULL;
}

void remove_block_node(metadata_entry_t *ptr) {
    if(ptr == NULL) {
        return;
    }
    if(ptr == metadata_head) {
        if(ptr->next != NULL) {
            ptr->next->prev = NULL;
        }
        metadata_head = ptr->next;
    } else if(ptr->prev != NULL && ptr->next != NULL) {
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
    } else if(ptr->prev != NULL) {
        ptr->prev->next = ptr->next;
    }
    ptr->prev = NULL;
    ptr->next = NULL;
}

void merge_block(metadata_entry_t *ptr) {
    metadata_entry_t *left_block = get_left_block(ptr);
    metadata_entry_t *right_block = get_right_block(ptr);
    if(left_block != NULL) {
        if(get_block_size(left_block) >= get_block_size(ptr) * MERGE_FACTOR) {
            size_t new_block_size = get_block_size(ptr) + get_block_size(left_block) + METADATA_SIZE + sizeof(int);
            set_block_size(left_block, new_block_size);
            return;
        }
    }
    if(right_block != NULL) {
        if(get_block_size(right_block) >= get_block_size(ptr) * MERGE_FACTOR) {
            remove_block_node(right_block);
            size_t new_block_size = get_block_size(ptr) + get_block_size(right_block) + METADATA_SIZE + sizeof(int);
            set_block_size(ptr, new_block_size);
        }
    }
    mark_block_free(ptr);
    ptr->next = metadata_head;
    if(metadata_head != NULL) {
        metadata_head->prev = ptr;
    }
    metadata_head = ptr;
}