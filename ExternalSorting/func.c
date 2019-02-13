#include <stdio.h>
#include "func.h"
#include "sort.h"
#include "BF.h"
#include "quicksort.h"
#include <string.h>

#if BLOCK_SIZE == 512
#define DEC 1
#endif
#if BLOCK_SIZE == 1024
#define DEC 2
#endif
#define MAXREC ((BLOCK_SIZE / 64) - DEC)

int write_block(int fileDesc, Record *record, int block_num, int rec_num, int list)
{
	int line,one=1,*num,nn,i,position;
	void *block;
	if(BF_ReadBlock(fileDesc,block_num,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}

	for(i=0;i<rec_num;i++)
	{
		position = i;
		line = position*64;
		block+=line;
		memcpy(block,&record[i],sizeof(struct Record));
		block-=line;
		block+=(MAXREC*64+(position)*sizeof(int));
		memcpy(block,&one,sizeof(int));
		block+=((MAXREC)-position)*sizeof(int);
		num=(int*)block;
		nn=*num;
		nn++;
		memcpy(block,&nn,sizeof(int));
		block-=(MAXREC*64+(MAXREC)*sizeof(int))*sizeof(char);
	}
	
	block+=(MAXREC*64+(MAXREC+1)*sizeof(int))*sizeof(char);
	memcpy(block,&list,sizeof(int));

	
	if (BF_WriteBlock(fileDesc,block_num) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	
}

int get_block(int fileDesc, Record *record, int block_num)
{
	int i,*list,t[MAXREC+2];
	void *block;
	if(BF_ReadBlock(fileDesc,block_num,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}

	for(i=0;i<MAXREC+2;i++)	t[i] = 0;
	t[MAXREC+1] = -1;
	for(i=0;i<MAXREC;i++)
	{
		memcpy(&record[i],block,sizeof(struct Record));
		block+=sizeof(struct Record);
	}
	block+=(MAXREC+1)*sizeof(int);
	list = (int*) block;
	//block-=(MAXREC+1)*sizeof(int);
	//memcpy(block,t,(MAXREC+2)*sizeof(int));
	return *list;

}

int print_block(int fileDesc, int block_num)
{
	void *block;
	int i,num;
	Record record;
	if(BF_ReadBlock(fileDesc,block_num,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	
	printf("Records of Block %d:\n", block_num);
	for(i=0;i<MAXREC;i++)
	{
		memcpy(&record,block,sizeof(struct Record));
		block+=sizeof(struct Record);
		print_record(record);
	}
	printf("Table and list of Block %d:\n",block_num);
	for(i=0;i<MAXREC;i++)
	{
		memcpy(&num,block,sizeof(int));
		block+=sizeof(int);
		printf("%d ",num);
	}
	memcpy(&num,block,sizeof(int));
	block+=sizeof(int);
	printf("sum=%d ",num);
	memcpy(&num,block,sizeof(int));	
	printf("list=%d\n",num);
	
}

int file_writeEnd(int bl_num,int fileDesc)
{
	void *block;
	Info info;
	if(BF_ReadBlock(fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	
	memcpy(&info,block,sizeof(struct Info));		//get info from block 0
	info.end_file = bl_num;						//add the end of file
	memcpy(block,&info,sizeof(struct Info));		//write back info to block 0
	
	if (BF_WriteBlock(fileDesc,0) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
}

int inner_sort(int fileDesc1, int fileDesc2, int block_read, int fieldNo)
{
	void *block;
	int i,info[MAXREC+2];
	Record record[MAXREC];
	if(BF_ReadBlock(fileDesc1,block_read,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	for(i=0;i<MAXREC;i++)
	{
		memcpy(&record[i],block,sizeof(struct Record));
		block+=sizeof(struct Record);
	}
	memcpy(info,block,(MAXREC+2)*sizeof(int));
	
	quickSort(0,MAXREC-1,record,fieldNo);
	
	Sorted_InsertEntry(fileDesc2, record, info);
}

void print_record(Record record)
{
	printf("%d,%s,%s,%s\n",record.id,record.name,record.surname,record.city);
}

int binary_search(Record* table, void *value, int FieldNo)
{
   int first, last, middle;
   int *val;
 
   val = (int *)value;
  
   first = 0;
   last = MAXREC - 1;
   middle = (first+last)/2;
  
   while (first <= last) 
   {
      if (FieldNo == 0)
      {       
         if (table[middle].id < (*val))
          first = middle + 1;    
         else if(table[middle].id == (*val))
         {  
            print_record(table[middle]); 
            return 1;
         }
         else last = middle - 1;
       }     
        else if (FieldNo == 1)
        {
            if (strcmp(table[middle].name,value)<0)
                first = middle+1;
            else if (strcmp(table[middle].name,value)==0)
            {  
                    return 1; 
            }
            else last = middle - 1;
        }
		else if (FieldNo == 2)
        {
            if (strcmp(table[middle].surname,value)<0)
                first = middle+1;
            else if (strcmp(table[middle].surname,value)==0)
            {  
                    return 1; 
            }
            else last = middle - 1;
        }
		else
        {
            if (strcmp(table[middle].city,value)<0)
                first = middle+1;
            else if (strcmp(table[middle].city,value)==0)
            {  
                    return 1; 
            }
            else last = middle - 1;
        }
        middle = (first + last)/2;
      }
       
   if (first > last) return 0;   
}
