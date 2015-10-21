
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ptr void*

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
}

void gc() {
    char stack_var;

    // gets stack (base?) pointer to current stack frame.
    // should be just a little bit more than the end of the main's stack frame.
    // It's only off by 16 bytes (good enough for now).
    void* main_stack_end = __builtin_frame_address(0);
    // gets stack pointer to previous stack frame (main). This should
    // be exactly the base pointer (confirmed within gdb).
    void* main_stack_start = __builtin_frame_address(1);

    // printf("main start: %p, main end: %p\n", main_stack_start, main_stack_end);

    heap_mem.end=sbrk(0);


    // now iterate over global and stack memory, performing mark on every pointer within
    // them that happens to point to anything in the heap.

}

// If p points to some word in an allocated block, returns a
// pointer b to the beginning of that block. Returns NULL otherwise.
ptr isPtr(ptr p) {

} 

// Returns true if block b is already marked.
int blockMarked(ptr b) {

}

// Returns true if block b is allocated.
int blockAllocated(ptr b) {

}

// Marks block b.
void markBlock(ptr b) {

}

// Returns the length in words (excluding the header) of block b.
int length(ptr b) {

}

// Changes the status of block b from marked to unmarked.
void unmarkBlock(ptr b) {

}

// Returns the successor of block b in the heap.
ptr nextBlock(ptr b) {

}

// TODO check the pointer types here
void mark(ptr p) {
    long* b;
    if ((b = isPtr(p)) == NULL)
        return;
    if (blockMarked(b))
        return;
    markBlock(b);
    int len = length(b);
    for (int i=0; i < len; i++)
        mark((void*) b[i]);
    return;
}

void sweep(ptr b, ptr end) {
    while (b < end) {
        if (blockMarked(b))
            unmarkBlock(b);
        else if (blockAllocated(b))
            free(b);
        b = nextBlock(b);
    }
    return;
}