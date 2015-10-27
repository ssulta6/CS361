#include <unistd.h>

typedef struct chunk {
	size_t size;
	struct chunk* next;
	struct chunk* prev;
} chunk;


void init_gc();
void gc();
size_t* isPtr(size_t* p);
int chunkMarked(size_t* b);
int chunkAllocated(size_t* b);
void markChunk(size_t* b);
long length(size_t* b);
void unmarkChunk(size_t* b);
size_t* nextChunk(size_t* b);
void mark(size_t* p);
void sweep(size_t* b, size_t* end);
void reset_heap_end();
