//
//  my_malloc-driver.c
//  Lab1: Malloc
//  This file is for testing your code only
//  You will not turn it in
//


#include "my_malloc.h"

int main(int argc, const char * argv[])

{   
    printf("coalesce test\n");

    char *ptr = my_malloc(424);   //432
    char *ptr1 = my_malloc(317);  //328 //free
    char *ptr2 = my_malloc(424);  //432 //free
    char *ptr3 = my_malloc(628);  //640 
    char *ptr4 = my_malloc(80);  //88
    char *ptr5 = my_malloc(20);  //32
    char *ptr6 = my_malloc(50);  //64
    char *ptr7 = my_malloc(16);  //24 //free
    char *ptr8 = my_malloc(100);  //112

    FreeListNode head0=free_list_begin();// the head is correct
    int size=head0->size;
    printf("     %lu\n",(char*)head0-ptr);// all memory allocated =2152-8=2144
    printf("size:%u\n",size);//remaining chunk size 6040


    my_free(ptr1);
    my_free(ptr2);
    coalesce_free_list();

    FreeListNode head=free_list_begin();
    int size1=head->size;
    printf("424 : %lu\n",(char*)head-ptr);// should be 424
    printf("size: %u\n",size1); // should be 328

    FreeListNode next=head->flink;
    int size2=next->size;
    printf("should be 328: %lu\n",(char*)next-(char*)head);// should be 432
    printf("shuld be 432:  %u\n",size2); // should be 328

    FreeListNode next2=next->flink;
    int size3=next2->size;
    printf("should be null: %lu\n",(char*)next2-ptr);// should be null
    printf("shuld be seg fault:  %u\n",size3); // should be seg fault




    // my_free(ptr2);
    // my_free(ptr3);
    // my_free(ptr7);
    // FreeListNode head=free_list_begin();
    // int size1=head->size;
    // printf("424 : %lu\n",(char*)head-ptr);
    // printf("size:%u\n",size1);
    // head=head->flink;
    // size=head->size;
    // if (head!=NULL){
    //     printf("problematic free list linking?\n");
    // }
    // printf("424 : %lu\n",(char*)head-ptr2);
    // printf("size:%u\n",size);
    // head=head->flink;
    // size=head->size;
    // printf("424 : %lu\n",(char*)head-ptr6);
    // printf("size:%u\n",size);
    // head=head->flink;
    // if (head!=NULL){
    //     printf("problematic ending maybe\n");
    // }
    
    printf("\nend\n");




    // my_free(ptr);
    // my_free(ptr1);
    // my_free(ptr2);
    // char *ptr2 = my_malloc(96);  //736
    
    // FreeListNode free1=free_list_begin();
    // printf("this is start of free  %p\n",free1);
    
    // char * interference=sbrk(10);
    // my_free(interference);
    
    // char *a = my_malloc(30);  //  40=32+8
    // FreeListNode free2=free_list_begin();
    // printf("this is start of free  %p\n",free2);

    
    
    // //This is my large allo test which is not working properlly, so I'm testing coalesce first
    // char *ptr3 = my_malloc(8192);  //8504
    // FreeListNode free0=free_list_begin();
    // printf("free list should be null : %p\n",free0);
    // char *ptr4 = my_malloc(80);  //88
    // FreeListNode free1=free_list_begin();
    // printf("free list should be 80 : %lu\n",(char*)free1-ptr4);
    // int size2=free1->size;
    // printf("size of free :%u\n",size2);
    // char *ptr5 = my_malloc(9020);  //9032
    // char *ptr6 = my_malloc(50);  //64
    // char *ptr7 = my_malloc(16);  //24
    // char *ptr8 = my_malloc(100);  //112

    // printf("%lu\n",ptr1-ptr);
    // printf("%lu\n",ptr2-ptr1);
    // printf("%lu\n",ptr3-ptr2);
    // printf("%lu\n",ptr4-ptr2);
    // printf("%lu\n",ptr5-ptr3);

    
    

    
    return 0;
}
