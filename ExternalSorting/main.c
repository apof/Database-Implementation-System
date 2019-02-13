#include "sort.h"
#include "func.h"
#include "BF.h"
#include <string.h>
#include <stdlib.h>

#if BLOCK_SIZE == 512
#define DEC 1
#endif
#if BLOCK_SIZE == 1024
#define DEC 2
#endif
#define MAXREC ((BLOCK_SIZE / 64) - DEC)

int main(int argc, char *argv[])
{
	int fileDescHeap,fileDescSort,id,counter,i,bl_num,check_sorted;
	size_t buf_size=70;
	char *name,*surname,*city,*filename,*buffer=malloc(buf_size*sizeof(char)),val[25];
	Record record[MAXREC];
	FILE *input_file;
	BF_Init();
	
	if(Heap_CreateFile("HEAP") != 0) {printf("Error in Heap_CreateIndex\n");return -1;}
	if((fileDescHeap = Heap_OpenFile("HEAP")) == -1) {printf("Error in Heap_OpenIndex\n");return -1;}
	if(argc == 1) 
	{
		printf("Give name of input file\n");
		return -1;
	}
	else
	{
		filename = malloc((strlen(argv[1])+1)*sizeof(char));
		strcpy(filename,argv[1]);
	}
	
	if((input_file = fopen(filename,"r")) == NULL) {printf("File can't open\n");return -1;}
	counter = 0;
	while(getline(&buffer,&buf_size,input_file) != -1)
	{
		id = atoi(strtok(buffer, ",\""));
		name = strtok(NULL,",\"");
		surname = strtok(NULL,",\"");
		city = strtok(NULL,",\"");
		record[counter].id = id;
		strcpy(record[counter].name,name);
		strcpy(record[counter].surname,surname);
		strcpy(record[counter].city,city);
		counter++;
		if(counter == MAXREC)
		{
			bl_num = Heap_Insert(fileDescHeap,record,MAXREC);
			counter = 0;
		}
	}
	if(counter != 0)
	{
		bl_num = Heap_Insert(fileDescHeap,record,counter);
	}
	file_writeEnd(bl_num,fileDescHeap);
	
	if(Heap_CloseFile(fileDescHeap) != 0) {printf("Error in Heap_CloseIndex\n");return -1;};

	int fieldNo;
	int *value=malloc(sizeof(int));
	//set parametres here
	fieldNo = 0;
	strcpy(filename,"HEAPSorted0");
	
	Sorted_SortFile("HEAP", fieldNo);	
	check_sorted = Sorted_checkSortedFile(filename,fieldNo);
	if( check_sorted == 0)	printf("File with name %s is sorted\n",filename);
	else if(check_sorted == -1)	printf("File with name %s is  NOT sorted\n",filename);
	printf("GetAllEntries\n\n");
	
	if((fileDescSort = BF_OpenFile(filename)) < 0)	
	{
		BF_PrintError("Error opening file");
		return -1;
	}
	*value = 11425351;
	strcpy(val,"Zula");
	Sorted_GetAllEntries(fileDescSort,fieldNo,value);
	
	fclose(input_file);
	if(Sorted_CloseFile(fileDescSort) != 0) {printf("Error in Sorted_CloseIndex\n");return -1;};

}
