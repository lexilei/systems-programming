#ifndef my_malloc_h
#define my_malloc_h
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//the size of the header for heap allocated memory chunks
#define CHUNKHEADERSIZE 8

//error signaling
typedef enum {MYNOERROR, MYENOMEM, MYBADFREEPTR} MyErrorNo;
extern MyErrorNo my_errno;

//my_malloc: returns a pointer to a chunk of heap allocated memory
void *my_malloc(uint32_t size);

//my_free: reclaims the previously allocated chunk referenced by ptr
void my_free(void *ptr);

//struct freelistnode: node for linked list of 'free' chunks
typedef struct freelistnode {
    struct freelistnode *flink; //pointer to next free chunk node
    uint32_t size; //size of current chunk
} * FreeListNode;

//free_list_begin(): returns pointer to first chunk in free list
FreeListNode free_list_begin(void);

//coalesce_free_list(): merge adjacent chunks on the free list
void coalesce_free_list(void);

#endif /* my_malloc_h */
