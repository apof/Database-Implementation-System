#ifndef func
#define func
#include "sort.h"
int write_block(int , Record *, int, int, int );
int get_block(int , Record *, int );
int print_block(int , int );
int file_writeEnd(int ,int );
int inner_sort(int , int, int, int);
int binary_search(Record* , void *, int );
void print_record(Record );
#endif