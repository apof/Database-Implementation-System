#include "sort.h"
#include "quicksort.h"
#include "func.h"
#include "BF.h"
#include <string.h>
#include <math.h>
#include <unistd.h>
#if BLOCK_SIZE == 512
#define DEC 1
#endif
#if BLOCK_SIZE == 1024
#define DEC 2
#endif
#define MAXREC ((BLOCK_SIZE / 64) - DEC)

int Heap_CreateFile(char *fileName)
{
	int fileDesc;
	void *block;
	if(BF_CreateFile(fileName) < 0) 	
	{
		BF_PrintError("Error creating file");
		return -1;
	}
	if((fileDesc = BF_OpenFile(fileName)) < 0)	
	{
		BF_PrintError("Error opening file");
		return -1;
	}
	if(BF_AllocateBlock(fileDesc) < 0)				
	{
		BF_PrintError("Error allocating file");
		return -1;
	}
	
	if(BF_CloseFile(fileDesc) != 0) 
	{
			printf("Error in Heap_CloseIndex\n");
			return -1;
	}

	return 0;
}

int Heap_OpenFile(char *fileName)
{
	int fileDesc;
	void *block;
	
	if((fileDesc = BF_OpenFile(fileName)) < 0)	
	{
		BF_PrintError("Error opening file");
		return -1;
	}
	
	if(BF_ReadBlock(fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	
	return fileDesc;
}

int Heap_CloseFile(int fileDesc)
{
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Error closing file");
		return -1;
	}
	return 0;
}

int Heap_Insert(int fileDesc, Record *record, int counter)
{
	int bl_num;
	if(BF_AllocateBlock(fileDesc) < 0)				
	{
		BF_PrintError("Error allocating file");
		return -1;
	}
	bl_num = BF_GetBlockCounter(fileDesc)-1;
	write_block(fileDesc, record, bl_num,counter,-1);
	return bl_num;
}

int Sorted_CreateFile(char *fileName, int fieldNo)
{
	int fileDesc;
	void *block;
	Info info;
	
	info.sorted_file = 1;
	info.fieldNo = fieldNo;
	
	if(BF_CreateFile(fileName) < 0) 	
	{
		BF_PrintError("Error creating file");
		return -1;
	}
	if((fileDesc = BF_OpenFile(fileName)) < 0)	
	{
		BF_PrintError("Error opening file");
		return -1;
	}
	if(BF_AllocateBlock(fileDesc) < 0)				
	{
		BF_PrintError("Error allocating file");
		return -1;
	}
	
	if(BF_ReadBlock(fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	memcpy(block,&info,sizeof(struct Info));
	if (BF_WriteBlock(fileDesc,0) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	
	if(BF_CloseFile(fileDesc) != 0) 
	{
			printf("Error in Sorted_CloseIndex\n");
			return -1;
	}
	return 0;
	
}

int Sorted_OpenFile(char *fileName)
{
	int fileDesc;
	void *block;
	Info info;
	
	if((fileDesc = BF_OpenFile(fileName)) < 0)	
	{
		BF_PrintError("Error opening file");
		return -1;
	}
	if(BF_ReadBlock(fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	memcpy(&info,block,sizeof(struct Info));
	if(info.sorted_file == 0)	return -1;
	
	return fileDesc;
}

int Sorted_CloseFile(int fileDesc)
{
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Error closing file");
		return -1;
	}
	return 0;
}

int Sorted_InsertEntry(int fileDesc, Record *record, int *info)
{
	int bl_num,i;
	void *block;
	
	if(BF_AllocateBlock(fileDesc) < 0)				
	{
		BF_PrintError("Error allocating file");
		return -1;
	}
	bl_num = BF_GetBlockCounter(fileDesc)-1;
	if(BF_ReadBlock(fileDesc,bl_num,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	for(i=0;i<MAXREC;i++)
	{
		memcpy(block,&record[i],sizeof(struct Record));
		block+=sizeof(struct Record);
	}
	memcpy(block,info,(MAXREC+2)*sizeof(int));

	if (BF_WriteBlock(fileDesc,bl_num) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	return 0;
}

int Sorted_SortFile(char *fileName, int fieldNo)
{
	int fileDescHeap,fileDescSort,i,heap_file_end,sort_file_end,bl_num,total_blocks,loop,step,start_block,end_block,inside_loop,j,block_to_read,flag,p1,p2,k,l,list1,list2,num1,num2;
	double temp;
	char new_fileName[30],*ch,buf[2];
	void *block,*block2;
	Info heap_info;
	Record in1[MAXREC],in2[MAXREC],out[MAXREC];
	sprintf(buf,"%d",fieldNo);
	strcpy(new_fileName,fileName);
	i=0;
	do
	{
		i++;
	}while(fileName[i] != '\0');
	new_fileName[i] = 'S';i++;
	new_fileName[i] = 'o';i++;
	new_fileName[i] = 'r';i++;
	new_fileName[i] = 't';i++;
	new_fileName[i] = 'e';i++;
	new_fileName[i] = 'd';i++;
	new_fileName[i] = buf[0];i++;
	new_fileName[i] = '\0';
	if((fileDescHeap = Heap_OpenFile(fileName)) == -1) {printf("Error in Heap_OpenIndex\n");return -1;}		//open heap file
	if(Sorted_CreateFile(new_fileName,fieldNo) != 0) {printf("Error in Sorted_CreateIndex\n");return -1;}			//create sorted file
	if((fileDescSort = Sorted_OpenFile(new_fileName)) == -1) {printf("Error in Sorted_OpenIndex\n");return -1;}		//open sorted file

	/*Esoteriki taksinomisi blocks*/
	
	if(BF_ReadBlock(fileDescHeap,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	memcpy(&heap_info,block,sizeof(struct Info));		//get heap file end
	heap_file_end = heap_info.end_file;
//printf("heapfileend=%d\n",heap_file_end);
	for(i=1;i<=heap_file_end;i++)
	{
		inner_sort(fileDescHeap,fileDescSort, i, fieldNo);
	}
	file_writeEnd(heap_file_end,fileDescSort);		//write sort_file_end to block 0
	
	sort_file_end = heap_file_end;
	
	start_block = 1;
	end_block = sort_file_end;
	total_blocks = (end_block) - (start_block) + 1;
	
	int list=-1;
	
	int power=1;
	do{
		power = 2*power;
	}while(power < total_blocks); 

	while( total_blocks < power )					//an ta total_blocks den einai artiou plhthous,prosthese allo ena block kai kane katallhlh arxikopoihsh				
	{
		sort_file_end++;
		total_blocks++;
		if(BF_AllocateBlock(fileDescSort) < 0)				
		{
			BF_PrintError("Error allocating file");
			return -1;
		}
		bl_num = BF_GetBlockCounter(fileDescSort)-1;
		if(BF_ReadBlock(fileDescSort,bl_num,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		block+=(MAXREC*64+(MAXREC+1)*sizeof(int))*sizeof(char);
		memcpy(block,&list,sizeof(int));
		if (BF_WriteBlock(fileDescSort,bl_num) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
	}
	
	temp = log2((double)total_blocks);
	if(temp - (int) temp == (double)0)	loop = (int) temp;
	else	
	{
		loop = (int) temp;
		loop++;
	}
//printf("loop=%d\n",loop);
	inside_loop = (int) total_blocks;
	
	int list_table[total_blocks],counter=0,block_to_write,fl,fileDescSort2,fileDescR,fileDescW;
	for(i=0;i<total_blocks;i++)	list_table[i] = start_block + i;
	
	/*****Temp SortFile for Merging*****/
	strcpy(new_fileName,"TEMP_SORT");
	if(Sorted_CreateFile(new_fileName,fieldNo) != 0) {printf("Error in Sorted_CreateIndex\n");return -1;}
	if((fileDescSort2 = Sorted_OpenFile(new_fileName)) == -1) {printf("Error in Sorted_OpenIndex\n");return -1;}
	for(i=0;i<total_blocks;i++)
	{
		if(BF_AllocateBlock(fileDescSort2) < 0)				
		{
			BF_PrintError("Error allocating file");
			return -1;
		}
	}
	/***********************************/
	for(i=0;i<loop;i++)
	{
		if( (i+1) % 2 == 0)	
		{
			fileDescR = fileDescSort2;
			fileDescW = fileDescSort;
		}
		else				
		{
			fileDescR = fileDescSort;
			fileDescW = fileDescSort2;
		}
		block_to_read = start_block;
		step = (int) pow(2.0,(double) i);
		flag = 0;
		block_to_write = 1;
		for(j=0;j<inside_loop;j++)
		{
			if(flag == 0)
			{
				fl = 1;
				list1 = get_block(fileDescR,in1,block_to_read);
				num1 = block_to_read;
				//block_to_write = num1;
				flag++;
			}
			else if(flag == 1)
			{
				list2 = get_block(fileDescR,in2,block_to_read);
				num2 = block_to_read;
				flag = 0;
				
				/**************Merge**************/
				p1 = 0;
				p2 = 0;
				for(l=0;l<(int) pow(2.0,(double) (i+1));l++)
				{
					for(k=0;k<MAXREC;k++)
					{
						if(p1 == MAXREC)
						{
							if(list1 != -1)
							{
								fl = 1;
								//block_to_write = num1;
								num1 = list1;
								list1 = get_block(fileDescR,in1,list1);			
								p1 = 0;
								if(fieldNo == 0)
								{
									if(p2==MAXREC)
									{
										out[k] = in1[p1];
										p1++;
									}
									else if(in1[p1].id < in2[p2].id)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
								else if(fieldNo == 1)
								{
									if(p2==MAXREC)
									{
										out[k] = in1[p1];
										p1++;
									}
									else if( strcmp(in1[p1].name,in2[p2].name) < 0)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
								else if(fieldNo == 2)
								{
									if(p2==MAXREC)
									{
										out[k] = in1[p1];
										p1++;
									}
									else if( strcmp(in1[p1].surname,in2[p2].surname) < 0)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
								else
								{
									if(p2==MAXREC)
									{
										out[k] = in1[p1];
										p1++;
									}
									else if( strcmp(in1[p1].city,in2[p2].city) < 0)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
							}
							else
							{
								if(fieldNo == 0)
								{	
									out[k] = in2[p2];
									p2++;
								}
								else
								{
									out[k] = in2[p2];
									p2++;
								}
							}
						}
						else if(p2 == MAXREC)
						{
							if(list2 != -1)
							{
								fl = 2;
								//block_to_write = num2;
								num2 = list2;
								list2 = get_block(fileDescR,in2,list2);			
								p2 = 0;
								if(fieldNo == 0)
								{
									if(p1==MAXREC)
									{
										out[k] = in2[p2];
										p2++;
									}
									else if(in1[p1].id < in2[p2].id)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
								else if(fieldNo == 1)
								{
									if(p1==MAXREC)
									{
										out[k] = in2[p2];
										p2++;
									}
									else if( strcmp(in1[p1].name,in2[p2].name) < 0)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
								else if(fieldNo == 2)
								{
									if(p1==MAXREC)
									{
										out[k] = in2[p2];
										p2++;
									}
									else if( strcmp(in1[p1].surname,in2[p2].surname) < 0)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}
								else
								{
									if(p1==MAXREC)
									{
										out[k] = in2[p2];
										p2++;
									}
									else if( strcmp(in1[p1].city,in2[p2].city) < 0)
									{
										out[k] = in1[p1];
										p1++;
									}
									else
									{
										out[k] = in2[p2];
										p2++;
									}
								}	
							}
							else
							{
								if(fieldNo == 0)
								{
									out[k] = in1[p1];
									p1++;
								}
								else
								{
									out[k] = in1[p1];
									p1++;
								}
							}
						}
						else
						{
							if(fieldNo == 0)
							{
								if(in1[p1].id < in2[p2].id)
								{
									out[k] = in1[p1];
									p1++;
								}
								else
								{
									out[k] = in2[p2];
									p2++;
								}
							}
							else if(fieldNo == 1)
							{
								if( strcmp(in1[p1].name,in2[p2].name) < 0)
								{
									out[k] = in1[p1];
									p1++;
								}
								else
								{
									out[k] = in2[p2];
									p2++;
								}
							}
							else if(fieldNo == 2)
							{
								if( strcmp(in1[p1].surname,in2[p2].surname) < 0)
								{
									out[k] = in1[p1];
									p1++;
								}
								else
								{
									out[k] = in2[p2];
									p2++;
								}
							}
							else
							{
								if( strcmp(in1[p1].city,in2[p2].city) < 0)
								{
									out[k] = in1[p1];
									p1++;
								}
								else
								{
									out[k] = in2[p2];
									p2++;
								}
							}	
						}
					}
					
					
					if(l != ((int) pow(2.0,(double) (i+1)))-1)	write_block(fileDescW,out,block_to_write,MAXREC,block_to_write+1);
					else	write_block(fileDescW,out,block_to_write,MAXREC,-1);
					
					counter++;
					block_to_write++;
				}
			}
			block_to_read += step;
		}
		counter = 0;
		
	
	
		inside_loop /= 2;
	}
	
	file_writeEnd(sort_file_end,fileDescSort);

	
	for(i=1;i<=sort_file_end;i++)
	{
		printf("*****\n");
		print_block(fileDescSort,i);
		printf("----\n");
	}
	
			

	if(Sorted_CloseFile(fileDescSort) != 0) {printf("Error in Sort_CloseIndex\n");return -1;};
	if(Sorted_CloseFile(fileDescSort2) != 0) {printf("Error in Sort_CloseIndex\n");return -1;};


}

int Sorted_checkSortedFile(char *fileName, int fieldNo)
{
	int i,j,fileDesc,file_end;
	Record record1,record2,prev_block_record;
	void *block;
	Info info;
	
	if((fileDesc = BF_OpenFile(fileName)) < 0)	
	{
		BF_PrintError("Error opening file");
		return -2;
	}
	if(BF_ReadBlock(fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -2;
	}
	memcpy(&info,block,sizeof(struct Info));		//get file end
	file_end = info.end_file;
	
	for(i=1;i<=file_end;i++)
	{

		if(BF_ReadBlock(fileDesc,i,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -2;
		}
		for(j=0;j<MAXREC-1;j++)
		{
			memcpy(&record1,block,sizeof(struct Record));
			block+=sizeof(struct Record);
			memcpy(&record2,block,sizeof(struct Record));
			if(fieldNo == 0)		//check for id
			{
				if(record1.id > record2.id)	{printf("%d %d\n",i,j);return -1;}		//not sorted
				if( (prev_block_record.id > record1.id) && (j == 0) && (i != 1))	{printf("%d %d\n",i,j);return -1;}		//sugkrisi me to teleutaio tou proigoumenou block
			}
			else if(fieldNo == 1)
			{
				if(strcmp(record1.name,record2.name) > 0)		{printf("%d %d\n",i,j);return -1;}
				if( (strcmp(prev_block_record.name,record1.name) > 0) && (j == 0) && (i != 1))	{printf("%d %d\n",i,j);return -1;}
			}
			else if(fieldNo == 2)
			{
				if(strcmp(record1.surname,record2.surname) > 0)	{printf("%d %d\n",i,j);return -1;}
				if( (strcmp(prev_block_record.surname,record1.surname) > 0) && (j == 0) && (i != 1))	{printf("%d %d\n",i,j);return -1;}
			}
			else 
			{
				if(strcmp(record1.city,record2.city) > 0)		{printf("%d %d\n",i,j);return -1;}
				if( (strcmp(prev_block_record.city,record1.city) > 0) && (j == 0) && (i != 1))	{printf("%d %d\n",i,j);return -1;}
			}
			if(j == MAXREC-2)	memcpy(&prev_block_record,&record2,sizeof(struct Record));		//krata thn teleytaia eggrafi tou block gia na thn sugkrinei me thn 1 tou epomenou block

		}
	}
	
	if(BF_CloseFile(fileDesc) != 0) 
	{
			printf("Error in Heap_CloseIndex\n");
			return -2;
	}
	
	return 0;
	
}

int Sorted_GetAllEntries(int fileDesc,int FieldNo,void *value)
{
     
    int file_end,i,j,start_block,end_block,middle,count,flag;
    int *val;
    Record record;
    void* block;
    Record table[MAXREC];
    Info info;

    val = (int*) value;
    if(BF_ReadBlock(fileDesc,0,&block) <0)
        {
           BF_PrintError("Error getting block");
           return -1;
        }
        memcpy(&info,block,sizeof(struct Info));    //arithmos block arxeiou
        file_end = info.end_file;
     
    if (val==NULL)     //ektypwse oles tis egrafes tou arxeiou ama dothei value NULL
    {
        for(i=1;i<=file_end;i++)
            {
                if(BF_ReadBlock(fileDesc,i,&block) <0)
                    {
                        BF_PrintError("Error getting block");
                        return -2;
                    }
                for(j=0;j<MAXREC;j++)
                    {
                        memcpy(&record,block,sizeof(struct Record));
                        print_record(record);
                        block+=sizeof(struct Record);
                    }
            }
     
    }
    else //ama den dothei value NULL kanei binary search sto arxeio
    {
        start_block=1;
        end_block=file_end;
         
        middle = (start_block + end_block)/2;
 
        while(start_block <= end_block)
        {
            if(BF_ReadBlock(fileDesc,middle,&block) <0)
                {
                    BF_PrintError("Error getting block");
                        return -2;
                }
            for(j=0; j<MAXREC; j++)
            {
                memcpy(&record,block,sizeof(struct Record));
                block+=sizeof(struct Record);
                table[j]=record;
            }
            if (FieldNo==0)
            {
              if(binary_search(table,value,0)==1) { printf("H eggrafh vrethhke!\n"); return 1; } 
              else
              {  
                 if ((*val) > table[MAXREC-1].id) start_block = middle +1;
                 else end_block = middle -1;                 
              }
            }
            else if (FieldNo==1) 
            {
              if(binary_search(table,value,1)==1) 
              { 
                printf("H eggrafh vrethhke!\n");
                for(j=0; j<MAXREC; j++)                                        //sto block pou vrhke to name tsekarei gia eggrafes me idio name
                 {
                   if (strcmp(table[j].name,value)==0) print_record(table[j]);
                 }
 
                 if (strcmp(table[MAXREC-1].name,value)==0)                    //ama to teleytaio name sympiptei tsekarei epomena block
                 {
                   count = middle+1;
                   do {
                       if(BF_ReadBlock(fileDesc,count,&block) <0)
                         {
                           BF_PrintError("Error getting block");
                           return -2;
                         }
                          
                       for(j=0; j<MAXREC; j++)
                        {
                         memcpy(&record,block,sizeof(struct Record));
                         if (strcmp(record.name,value)==0) print_record(record);
                         if (j==MAXREC-1)
                         {
                          if (strcmp(record.name,value)==0) { count++; flag=1; }
                          else flag=0;
                         }
                         block+=sizeof(struct Record);
                        }
                       
                   }while ((flag==1) && (count<=file_end));
                 }
              
              
                 if (strcmp(table[0].name,value)==0)
                 {
                   count = middle-1;
                   do {
                       if(BF_ReadBlock(fileDesc,count,&block) <0)
                         {
                           BF_PrintError("Error getting block");
                           return -2;
                         }
                             block+=(MAXREC-1)*sizeof(struct Record);                                                 
                       for(j=MAXREC-1; j>=0; j--)
                        {
                         memcpy(&record,block,sizeof(struct Record));
                         if (strcmp(record.name,value)==0) print_record(record);
                         if (j==0)
                         {
                          if (strcmp(record.name,value)==0) { count--; flag=1; }
                          else flag=0;
                         }
                         block-=sizeof(struct Record);
                        }
                       
                   }while ((flag==1) && (count != 0));
                 }
              
                return 1;                   
              } 
              else
              {  
                 if ( strcmp(value,table[MAXREC-1].name)>0) start_block = middle +1;
                 else end_block = middle -1;                 
              }
                 
            }
            else if (FieldNo==2) 
			{
				if(binary_search(table,value,2)==1) 
              { 
                printf("H eggrafh vrethhke!\n");
                for(j=0; j<MAXREC; j++)                                        //sto block pou vrhke to name tsekarei gia eggrafes me idio name
                 {
                   if (strcmp(table[j].surname,value)==0) print_record(table[j]);
                 }
 
                 if (strcmp(table[MAXREC-1].surname,value)==0)                    //ama to teleytaio name sympiptei tsekarei epomena block
                 {
                   count = middle+1;
                   do {
                       if(BF_ReadBlock(fileDesc,count,&block) <0)
                         {
                           BF_PrintError("Error getting block");
                           return -2;
                         }
                          
                       for(j=0; j<MAXREC; j++)
                        {
                         memcpy(&record,block,sizeof(struct Record));
                         if (strcmp(record.surname,value)==0) print_record(record);
                         if (j==MAXREC-1)
                         {
                          if (strcmp(record.surname,value)==0) { count++; flag=1; }
                          else flag=0;
                         }
                         block+=sizeof(struct Record);
                        }
                       
                   }while ( (flag==1) && (count<=file_end));
                 }
              
              
                 if (strcmp(table[0].surname,value)==0)
                 {
                   count = middle-1;
                   do {
                       if(BF_ReadBlock(fileDesc,count,&block) <0)
                         {
                           BF_PrintError("Error getting block");
                           return -2;
                         }
                           block+=(MAXREC-1)*sizeof(struct Record);                         
                       for(j=MAXREC-1; j>=0; j--)
                        {
                         memcpy(&record,block,sizeof(struct Record));
                         if (strcmp(record.surname,value)==0) print_record(record);
                         if (j==0)
                         {
                          if (strcmp(record.surname,value)==0) { count--; flag=1; }
                          else flag=0;
                         }
                         block-=sizeof(struct Record);
                        }
                       
                   }while ( (flag==1) && (count!=0));
                 }
              
                return 1;                   
              } 
              else
              {  
                 if ( strcmp(value,table[MAXREC-1].surname)>0) start_block = middle +1;
                 else end_block = middle -1;                 
              }
			}
            else 
			{
				if(binary_search(table,value,3)==1) 
              { 
                printf("H eggrafh vrethhke!\n");
                for(j=0; j<MAXREC; j++)                                        //sto block pou vrhke to name tsekarei gia eggrafes me idio name
                 {
                   if (strcmp(table[j].city,value)==0) print_record(table[j]);
                 }
 
                 if (strcmp(table[MAXREC-1].city,value)==0)                    //ama to teleytaio name sympiptei tsekarei epomena block
                 {
                   count = middle+1;
                   do {
                       if(BF_ReadBlock(fileDesc,count,&block) <0)
                         {
                           BF_PrintError("Error getting block");
                           return -2;
                         }
                          
                       for(j=0; j<MAXREC; j++)
                        {
                         memcpy(&record,block,sizeof(struct Record));
                         if (strcmp(record.city,value)==0) print_record(record);
                         if (j==MAXREC-1)
                         {
                          if (strcmp(record.city,value)==0) { count++; flag=1; }
                          else flag=0;
                         }
                         block+=sizeof(struct Record);
                        }   
                   }while ( (flag==1) && (count<=file_end) );
                 }
              
              
                 if (strcmp(table[0].city,value)==0)
                 {
                   count = middle-1;
                   do {
                       if(BF_ReadBlock(fileDesc,count,&block) <0)
                         {
                           BF_PrintError("Error getting block");
                           return -2;
                         }
                          block+=(MAXREC-1)*sizeof(struct Record);
                       for(j=MAXREC-1; j>=0; j--)
                        {
                         memcpy(&record,block,sizeof(struct Record));
                         if (strcmp(record.city,value)==0) print_record(record);
                         if (j==0)
                         {
                          if (strcmp(record.city,value)==0) { count--; flag=1; }
                          else flag=0;
                         }
                         block-=sizeof(struct Record);
                        }
                   }while ( (flag==1) && (count != 0));
                 }
              
                return 1;                   
              } 
              else
              {  
                 if ( strcmp(value,table[MAXREC-1].city)>0) start_block = middle +1;
                 else end_block = middle -1;                 
              }
			}
             
        middle = (start_block + end_block)/2;   
        }
    }
}


