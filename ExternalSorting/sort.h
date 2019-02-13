#ifndef sort
#define sort
#include <stdio.h>

typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[25];
} Record;

typedef struct Info{
	int sorted_file;
	int fieldNo;
	int end_file;
}Info;

int Sorted_CreateFile(char *,int);
int Sorted_OpenFile(char *);
int Sorted_CloseFile(int );
int Sorted_InsertEntry(int , Record *, int *);
int Sorted_SortFile(char *, int );
int Sorted_checkSortedFile(char *, int );
int Sorted_GetAllEntries(int ,int ,void *);


#endif
