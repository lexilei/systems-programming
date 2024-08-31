//
//  my_malloc.c
//  Lab1: Malloc

// your Emory email address/user ID: zlei8

/* THIS CODE WAS MY OWN WORK , IT WAS WRITTEN WITHOUT CONSULTING ANY
SOURCES OUTSIDE OF THOSE APPROVED BY THE INSTRUCTOR . Lex Lei */


//

#include "my_malloc.h"
#include <unistd.h>//not all of these packages are used, I'm using only sbrk and NULL, used printf for testing but deleted them in the end
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAGIC_CODE 3159//in hex it's C57

MyErrorNo my_errno=MYNOERROR;

FreeListNode f = NULL;

void *my_malloc(uint32_t size) {
    uint32_t totalSize = size + CHUNKHEADERSIZE;
    if (totalSize<16)
        totalSize=16;
    else{
        totalSize = (totalSize + 7) & ~7;} 

    if (totalSize>=8192){
        void *ptr=sbrk(0);
        sbrk(totalSize);
        *((int*)ptr) = totalSize;
        *((int*)ptr+1) = MAGIC_CODE;
        return ptr+CHUNKHEADERSIZE;
    }

    FreeListNode previous = NULL;
    void *ptr;

    if (f==NULL){
        f=sbrk(0);
        sbrk(8192);
        f->size=8192;
        f->flink=NULL; // initializing first all empty chunk of memory
    }

    //traverse through current free list
    FreeListNode cur=f;
    while (cur!=NULL){
        if (cur->size >= totalSize){//found usable block
            uint32_t remainder = cur->size - totalSize;

            if (remainder >= 16) {//case1
                // Split the chunk and create a new free list node with the remaining part
                FreeListNode new_node = (void *)((uint8_t *)cur + totalSize);
                new_node->size = remainder;
                new_node->flink = cur->flink;
                // Update the current chunk size because it's now smaller
                if (previous) {
                    previous->flink = new_node;
                } else {
                    f = new_node;
                }
                
            }
            else{//case2
                // Remove the whole chunk from the free list
                totalSize=cur->size;

                if (previous) {
                    previous->flink = cur->flink;
                } else {
                    f = cur->flink;
                }
            }
            ptr=cur;
            *((int*)ptr) = totalSize;
            *((int*)ptr+1) = MAGIC_CODE;
            return ptr+CHUNKHEADERSIZE;
        }
        previous=cur;
        cur=cur->flink;
    }

    //no cur chunk in free list is big enough, so we creat new chunks here
    uint32_t availableSize=0;
    cur=sbrk(0);
    ptr=cur;
    void* new_space;

    if (totalSize>=8192){
        new_space=sbrk(totalSize);
        if (new_space == (void*)-1) {//catch error
        my_errno=MYENOMEM;
        return NULL;
        }
        availableSize=totalSize;
    }else{
        new_space=sbrk(8192);
        if (new_space == (void*)-1) {//catch error
        my_errno=MYENOMEM;
        return NULL;
    }
        availableSize=8192;
    }
    
    uint32_t remainder = availableSize - totalSize;
    if (remainder >= 16){
        FreeListNode new_node = (FreeListNode)((uint8_t *)cur + totalSize);
        new_node->size = remainder;
        new_node->flink=NULL;
        if (previous) {
            previous->flink = new_node;
        } else {
            f = new_node;
        }
        *((int*)ptr) = totalSize;
        *((int*)ptr+1) = MAGIC_CODE;
        
        return ptr+CHUNKHEADERSIZE;
    }
    
    *((int*)ptr) = totalSize;
    *((int*)ptr+1) = MAGIC_CODE;
    
    return ptr+CHUNKHEADERSIZE;//this is the address we return, the header is for us, not the user
}


      
void my_free(void *ptr){
    if (ptr == NULL) {
        my_errno=MYBADFREEPTR;
        return;
    }
    //check magic code
    int checkMagicCode;
    checkMagicCode=*((int*)ptr-1);
    if (checkMagicCode!=MAGIC_CODE) {
        my_errno=MYBADFREEPTR;
        return;
    }

    //traverse through free list to find where should ptr be located in the list
    FreeListNode cur=f;
    FreeListNode prev=NULL;
    
    while (cur!=NULL){

        if ((char *)cur>(char *)ptr){
            break;
        }
        prev=cur;
        cur=cur->flink;
    }

    FreeListNode x;//this is the new node
    x=(FreeListNode)((uint8_t *)ptr-CHUNKHEADERSIZE);
    x->size=*((int*)ptr-2);
    x->flink=cur; 
    if (f==NULL){//if no head
        f=x;
        return;
    }
    if (prev!=NULL) prev->flink=x;
    else{
        f=x;
    }
    

}

FreeListNode free_list_begin(){
    return f;
}

void coalesce_free_list(){
    FreeListNode cur=f;
    if (f==NULL) return;
//traverse through entire free list, if merged, we don't move forward, in case next node should still be merged
    int size;
    char * address;
    char * next;
    while (cur!=NULL){
        size=cur->size;
        address=(char *)cur;
        next=(char *)(cur->flink);
        if (address+size==next){
            cur->size=size+cur->flink->size;
            cur->flink=cur->flink->flink;
        }
        else cur=cur->flink;
    }
}
