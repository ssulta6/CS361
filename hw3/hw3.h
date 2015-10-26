#include <unistd.h>

typedef struct chunk {
	size_t size;
	struct chunk* next;
	struct chunk* prev;
} chunk;


void init_gc();
void gc();
size_t* isPtr(size_t* p);
int blockMarked(size_t* b);
int blockAllocated(size_t* b);
void markBlock(size_t* b);
long length(size_t* b);
void unmarkBlock(size_t* b);
size_t* nextBlock(size_t* b);
void mark(size_t* p);
void sweep(size_t* b, size_t* end);
