
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
}

void gc() {
    char stack_var;
    heap_mem.end=sbrk(0);
}
