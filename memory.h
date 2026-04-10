#ifndef MEMORY_H
#include <stdint.h>
#include <stddef.h>

//Struct for block
typedef struct block {
    size_t size;
    int free;
    struct block* next;
} block_t;

void init_heap();
void free(void* ptr);
block_t* find_free_block(size_t size);
void split_block(block_t* block, size_t size);
void* malloc(size_t size) ;
#endif