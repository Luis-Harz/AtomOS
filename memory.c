//Finally malloc and free, yay
#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "vga.h"

//Variables and things
//typedef unsigned int size_t;
block_t* block = (block_t*)NULL;
#define BLOCK_SIZE sizeof(block_t)
#define HEAP_SIZE 1024 * 10240
static char heap[HEAP_SIZE];
block_t* free_list = (block_t*)NULL;

//Functions
void init_heap() {
    free_list = (block_t*)heap;
    free_list->size = HEAP_SIZE - BLOCK_SIZE;
    free_list->free = 1;
    free_list->next = (block_t*)NULL;
}

void free(void* ptr) {
    if (!ptr) return;
    block_t* block = (block_t*)((char*)ptr - BLOCK_SIZE);
    block->free = 1;
    block_t* current = free_list;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

block_t* find_free_block(size_t size) {
    block_t* current = free_list;

    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }

    return (block_t*)NULL;
}

void split_block(block_t* block, size_t size) {
    block_t* new_block = (block_t*)((char*)block + BLOCK_SIZE + size);
    new_block->size = block->size - size - BLOCK_SIZE;
    new_block->free = 1;
    new_block->next = block->next;
    block->size = size;
    block->next = new_block;
}

void* malloc(size_t size) {
    if (size <= 0) return NULL;
    block_t* block = find_free_block(size);
    if (!block) return NULL;

    if (block->size > size + BLOCK_SIZE) {
        split_block(block, size);
    }
    block->free = 0;
    void* ptr = (void*)((char*)block + BLOCK_SIZE);
    uintptr_t aligned = ((uintptr_t)ptr + 3) & ~3;
    return (void*)aligned;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    block_t* block = (block_t*)((char*)ptr - BLOCK_SIZE);
    if (block->size >= size) {
        return ptr;
    }
    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    for (size_t i = 0; i < block->size; i++) {
        ((char*)new_ptr)[i] = ((char*)ptr)[i];
    }
    free(ptr);

    return new_ptr;
}