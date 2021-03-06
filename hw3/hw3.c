#include "hw3.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

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
            if ((size_t*)start==global_mem.end){
                global_mem.end=(size_t*)end;
            }
            break;
        }
    }
    fclose(mapfile);
}

void init_gc() {
    init_global_range();
    heap_mem.start=(size_t*)malloc(512);

    // gets stack pointer to previous stack frame (main). This should
    // be exactly the base pointer (confirmed within gdb).
    stack_mem.start = (size_t*)__builtin_frame_address(1);
}

void reset_heap_end() {
    heap_mem.end = (size_t*)sbrk(0);
}

void gc() {
    char stack_var;

    // gets stack (base?) pointer to current stack frame.
    // should be just a little bit more than the end of the main's stack frame.
    stack_mem.end = (size_t*)__builtin_frame_address(0) + 16;

    heap_mem.end = sbrk(0);


    // now iterate over global and stack memory, performing mark on every pointer within
    // them that happens to point to anything in the heap.

    // go over global memory and mark all reachable heap memory
    for (size_t* current_global = global_mem.start; current_global < global_mem.end; current_global++) {
        size_t* current_chunk = isPtr((size_t*)*current_global);

        // if not NULL and we're not looping inside of the heap, then mark it recursively
        if (current_chunk && (current_global < heap_mem.start || current_global > heap_mem.end)) {
            mark(current_chunk);
        }

    }

    // go over stack memory (from start to end, by decrementing) and mark all reachable heap memory
    for (size_t* current_stack = stack_mem.start; current_stack > stack_mem.end; current_stack--) {
        size_t* current_chunk = isPtr((size_t*)*current_stack);
        // if not NULL
        if (current_chunk) {
            mark(current_chunk);
        }

    }


    // now go over ALL heap memory and remove unmarked blocks
    sweep(heap_mem.start, heap_mem.end);
}

// If p points to some word in an allocated chunk, returns a
// pointer b to the beginning of that chunk. Returns NULL otherwise.
size_t* isPtr(size_t* p) {

    // return NULL if p is NULL
    if (!p) {
        return NULL;
    }
    // first check whether it's in range of heap memory (exclude last block)
    if (p < heap_mem.start || p >= heap_mem.end) {
        return NULL;
    }

    // find the header for this chunk
    // traverse entire heap and find the header for this chunk
    size_t* current_mem = heap_mem.start;  // points to mem section of current chunk
    while (current_mem < heap_mem.end) {
        size_t* current_chunk = current_mem-2;  // points to header section of current chunk
        // now check if the pointer in question is between current and next chunk
        size_t* next_mem = nextChunk(current_chunk) + 2;
        if (current_mem <= p && p < next_mem)
            return current_chunk;  // return header to this chunk
        
        current_mem = next_mem;  // move on to next chunk
    }

    return NULL;
} 

// Returns true if block b is already marked.
// assumes b is a pointer to chunk (header of this chunk)
int chunkMarked(size_t* b) {
    size_t* tmp = (b+1);
    return (long)(*tmp) & 0b100;
}

// Returns true if block b is allocated.
// assumes b is pointer to chunk (header of this chunk)
int chunkAllocated(size_t* b) {
    size_t* next_chunk = nextChunk(b);
    // avoid segfaults by checking if the given pointer is within heap
    if (next_chunk < heap_mem.start || next_chunk >= heap_mem.end)
        return 0;
    // the least sig. bit of next_chunk+1 has current chunk allocated bit
    return (long)(*(next_chunk + 1)) & 1;
}

// Marks chunk b, assumes b is a pointer to chunk (header of this chunk)
void markChunk(size_t* b) {
    // set the 3rd bit of the 2nd block
    size_t* tmp = (b+1);
    *tmp = (long)(*tmp) | 0b100;
}

// Returns the length in words (including the header) of block b.
// assumes b is pointer to chunk (header of this chunk)
long length(size_t* b) {

    // b-1 gives the chunk size, we need to remove lower three bits cuz flags
    return (long)(*(b + 1)) >> 3;  // return size in words (8 bytes each)
}

// Changes the status of block b from marked to unmarked.
// assumes b is pointer to chunk (header of this chunk)
void unmarkChunk(size_t* b) {
    // unset the 3rd bit of the 2nd block
    size_t* tmp = (b+1);
    *tmp = (long)(*tmp) & ~0b100;
}

// Returns pointer to mem of next chunk in the heap.
// assumes b is pointer to chunk (header of this chunk)
size_t* nextChunk(size_t* b) {
    return b + length(b);
}

// recursively mark this chunk and any heap pointer inside this chunk.
// current_chunk will hold pointer to chunk (header of the chunk)
void mark(size_t* current_chunk) {
    if (!current_chunk)
        return;
    if (chunkMarked(current_chunk))
        return;
    markChunk(current_chunk);
    // len is length of entire chunk minus header
    int len = length(current_chunk) - 2;
    size_t* current_mem = current_chunk + 2;
    // now call this recursively on every block in this chunk (within mem user data)
    for (int i=0; i < len; i++) {
        size_t* next_chunk = isPtr((size_t*)*(current_mem + i));
        mark(next_chunk);
    }
}

// go over entire heap range and free unmarked chunks
void sweep(size_t* current_mem, size_t* end) {
    
    while (current_mem < end) {

        size_t* current_chunk = current_mem-2;  // points to header section of current chunk
        
        // now check if the pointer in question is between current and next chunk
        size_t* next_mem = nextChunk(current_chunk) + 2;

        // if current chunk is marked, unmark it so we reset for the next gc() call 
        if (chunkMarked(current_chunk)) {
            unmarkChunk(current_chunk);
        // if current chunk is unmarked AND allocated, then we can free it (give it mem pointer)
        } else if (chunkAllocated(current_chunk)) {
            free(current_mem);
        }
        
        current_mem = next_mem;  // move on to next chunk
        end = sbrk(0);  // update heap end in case the OS shrinks it after a free
    }
}

