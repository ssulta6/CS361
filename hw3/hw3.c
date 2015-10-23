#include "hw3.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct memory_region{
    size_t * start;
    size_t * end;
};

struct memory_region global_mem;
struct memory_region heap_mem;
struct memory_region stack_mem;

//grabbing the address and size of the global memory region from proc 
void init_global_range(){
    char file[100];
    char * line=NULL;
    size_t n=0;
    size_t read_bytes=0;
    size_t start, end;
    
    sprintf(file, "/proc/%d/maps", getpid());
    FILE * mapfile  = fopen(file, "r");
    if (mapfile==NULL){
        perror("opening maps file failed\n");
        exit(-1);
    }
    
    int counter=0;
    while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
        if (strstr(line, "hw3")!=NULL){
            ++counter;
            if (counter==3){
                sscanf(line, "%lx-%lx", &start, &end);
                global_mem.start=(size_t*)start;
                global_mem.end=(size_t*)end;
            }
        }
        else if (read_bytes > 0 && counter==3){
            //if the globals are larger than a page, it seems to create a separate mapping
            sscanf(line, "%lx-%lx", &start, &end);
            if (start==global_mem.end){
                global_mem.end=(size_t*)end;
            }
            break;
        }
    }
    fclose(mapfile);
}

void init_gc() {
    init_global_range();
    heap_mem.start=malloc(512);

    // gets stack pointer to previous stack frame (main). This should
    // be exactly the base pointer (confirmed within gdb).
    stack_mem.start = __builtin_frame_address(1);
}

void gc() {
    char stack_var;

    // gets stack (base?) pointer to current stack frame.
    // should be just a little bit more than the end of the main's stack frame.
    // It's only off by 16 bytes (good enough for now).
    stack_mem.end = __builtin_frame_address(0);

    printf("main start: %p, main end: %p\n", stack_mem.start, stack_mem.end);

    heap_mem.end=sbrk(0);


    // now iterate over global and stack memory, performing mark on every pointer within
    // them that happens to point to anything in the heap.

    // go over global memory and mark all reachable heap memory
    for (size_t* current_global = global_mem.start; current_global < global_mem.end; current_global++) {
        // printf("isPtr(%lx): %lx\n", (unsigned long) current_global, (unsigned long)isPtr(current_global));
        size_t* p = isPtr((size_t*)(*current_global));
        // if not NULL
        if (p) {
            // printf("global isPtr(%p): %p\n", current_global, p);
            mark(p);
        }

    }

    // go over stack memory and mark all reachable heap memory
    for (size_t* current_stack = stack_mem.start; current_stack > stack_mem.end; current_stack--) {
        size_t* p = isPtr((size_t*)(*current_stack));
        // if not NULL
        if (p) {
            // printf("stack isPtr(%p): %p\n", current_stack, p);
            mark(p);
        } else {
            // printf("stack ptr %p is null!\n", current_stack);
        }

    }

}

// If p points to some word in an allocated block, returns a
// pointer b to the beginning of that block. Returns NULL otherwise.
size_t* isPtr(size_t* p) {
    // first check whether it's in range of heap memory (exclude last block)
    if (p < heap_mem.start || p >= heap_mem.end) {
        // printf("pointer %lx is not in heap memory range (%lx to %lx)!\n", (unsigned long)p, (unsigned long) heap_mem.start, (unsigned long) heap_mem.end);
        return NULL;
    }

    printf("pointer %p is in heap range!\n", p);
    // now check that the block is allocated
    if (blockAllocated(p)) {
        return p-2;
    }

    return NULL;
} 

// Returns true if block b is already marked.
int blockMarked(size_t* b) {
    size_t* tmp = (b+1);
    return *tmp & 0b100;
}

// Returns true if block b is allocated.
// assumes b is pointer to mem (user data)
int blockAllocated(size_t* b) {
    size_t* next_chunk = b-1 + length(b);
    // the least sig. bit of next_chunk has current chunk allocated bit
    return (long)next_chunk & 1;
}

// Marks block b.
void markBlock(size_t* b) {
    // set the 3rd bit of the 2nd block
    size_t* tmp = (b+1);
    *tmp = (*tmp | 0b100);
}

// Returns the length in words (excluding the header) of block b.
int length(size_t* b) {
    // b-1 gives the chunk size, we need to remove lower three bits cuz flags
    return (long)(b - 1) & ~7;
}

// Changes the status of block b from marked to unmarked.
void unmarkBlock(size_t* b) {
    // unset the 3rd bit of the 2nd block
    size_t* tmp = (b+1);
    *tmp = *tmp & ~0b100;
}

// Returns the successor of block b in the heap.
size_t* nextBlock(size_t* b) {

}

void mark(size_t* p) {
    size_t* b;
    if ((b = isPtr(p)) == NULL)
        return;
    if (blockMarked(b))
        return;
    markBlock(b);
    int len = length(b);
    for (int i=0; i < len; i++)
        mark((size_t *)b[i]);
    return;
}

void sweep(size_t* b, size_t* end) {
    while (b < end) {
        if (blockMarked(b))
            unmarkBlock(b);
        else if (blockAllocated(b))
            free(b);
        b = nextBlock(b);
    }
    return;
}