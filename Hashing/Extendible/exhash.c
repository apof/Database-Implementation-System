#include "BF.h"
#include "exhash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#if BLOCK_SIZE == 512
#define DEC 1
#endif
#if BLOCK_SIZE == 1024
#define DEC 2
#endif
#define MAXREC ((BLOCK_SIZE / 64) - DEC)

void Info_print(EH_info* info){
	printf("fileDesc = %d\n",info->fileDesc);
	printf("attrType = %c\n", info->attrType);
	printf("attrName = %s\n", info->attrName);
	printf("attrLength = %d\n", info->attrLength);
	printf("Depth = %d\n", info->depth);}
	
	
int EH_CreateIndex(char *fileName, char* attrName, char attrType, int attrLength, int depth) {
    int fileDesc,str_len,i,j,table_block_size;
	int init = -1,init2 = -2,n;
	void *block;
	char ch='0',ch2='\0';
	table_block_size = ((int) pow(2.0,(double)depth)) / (BLOCK_SIZE / sizeof(int)-1);
	if((int) pow(2.0,(double)depth) % (BLOCK_SIZE / sizeof(int)-1) != 0)	table_block_size++;
	
	if(BF_CreateFile(fileName) < 0) 	
	{
		BF_PrintError("Error creating file");
		return -1;
	}
	if(fileDesc = BF_OpenFile(fileName) < 0)	
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
	strncpy(block,&attrType,sizeof(char));
	block += sizeof(char);
	str_len = strlen(attrName);
	strncpy(block,attrName,attrLength*sizeof(char));
	if(str_len < attrLength)
	{
		block += str_len*sizeof(char);
		for(i=str_len;i<attrLength;i++)	
		{
			if(i==str_len) strncpy(block,(char*)&ch2,sizeof(char));
			else strncpy(block,(char*)&ch,sizeof(char));
			block+=sizeof(char);
		}
	}
	else	block += attrLength*sizeof(char);
	strncpy(block,(char*)&attrLength,sizeof(int));
	block += sizeof(int);
	strncpy(block,(char*)&depth,sizeof(int));
	
	if (BF_WriteBlock(fileDesc,0) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}

	/*Allocate and Initialize block/blocks for hash table*/
	for(i=1;i<=table_block_size;i++)
	{
//printf("Block with number %d created\n",i);
		if(BF_AllocateBlock(fileDesc) < 0)				
		{
			BF_PrintError("Error allocating file");
			return -1;
		}
		if(BF_ReadBlock(fileDesc,i,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		if((i-1)*(BLOCK_SIZE / sizeof(int)) < (int) pow(2.0,(double)depth) && (table_block_size!=1))
		{
			for(j=0;j<(BLOCK_SIZE / sizeof(int));j++)	
			{
				n = i+1;
				if((j==0) && (i != table_block_size))	
				{
					strncpy(block,(char*)&n,sizeof(int));
					block+=sizeof(int);
				}
				strncpy(block,(char*)&init,sizeof(int));
				block+=sizeof(int);
			}
		}
		else
		{
			for(j=0;j<=(int) pow(2.0,(double)depth)-(i-1)*(BLOCK_SIZE / sizeof(int));j++)	
			{
				/*n = i+1;
				if((j==0) && (i != table_block_size))	
				{
					strncpy(block,(char*)&n,sizeof(int));
					block+=sizeof(int);
				}*/
				strncpy(block,(char*)&init,sizeof(int));
				block+=sizeof(int);
			}
			for(j=(int) pow(2.0,(double)depth)+1-(i-1)*(BLOCK_SIZE / sizeof(int));j<(BLOCK_SIZE / sizeof(int));j++)
			{
				strncpy(block,(char*)&init2,sizeof(int));
				block+=sizeof(int);
			}
		}

		if (BF_WriteBlock(fileDesc,i) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
	}
    
	return 0;
   
}


EH_info* EH_OpenIndex(char *fileName) {

	int fileDesc;
	void *block;
	EH_info *info = malloc(sizeof(EH_info));
	char *str;
	int *k;
	
	if(fileDesc = BF_OpenFile(fileName) < 0)	
	{
		BF_PrintError("Error opening file");
		return NULL;
	}
	
	if(BF_ReadBlock(fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return NULL;
	}
	str = (char*) block;
	info->fileDesc = fileDesc;
	strncpy(&info->attrType,str,sizeof(char));
	block+=sizeof(char);
	str = (char*) block;
	block+=10*sizeof(char);
	k=(int*) block;
	info->attrName = malloc((*k)*sizeof(char)+1);
	strncpy(info->attrName,str,(*k)*sizeof(char));
	info->attrName[(*k)*sizeof(char)] = '\0';
	info->attrLength = *k;
	block+=sizeof(int);
	k=(int*) block;
	info->depth = (long int) *k;	
	
    return info;
} 



int EH_CloseIndex(EH_info* header_info) {

	if (BF_CloseFile(header_info->fileDesc) < 0) {
		BF_PrintError("Error closing file");
		return -1;
	}
	free(header_info->attrName);
	free(header_info);
    
    return 0;  
}

int EH_InsertEntry(EH_info* header_info, Record record) {
    
	static int init_val = 0;
	int hash_number,block_num,count=0,*block_to_read=malloc(sizeof(int)),*bucket,*l,i,*val,zero=0,*flag,next,init=-1,init2=-2;
	int table_block_size,j,n,*local_depth,over_flow_point,*t,previous_list,btr=0,*k;
	void *block;
	
	if(header_info->attrType == 'i')	hash_number = hash_func(record.id,header_info->depth);		//key is integer
	else if(strcmp(header_info->attrName,"name") == 0)	hash_number = hash_func_string(record.name,header_info->depth);
	else if(strcmp(header_info->attrName,"surname") == 0)	hash_number = hash_func_string(record.surname,header_info->depth);
	else if(strcmp(header_info->attrName,"city") == 0)	hash_number = hash_func_string(record.city,header_info->depth);
	else{printf("there is no such key\n");return -1;}
	//printf("HASH NUMBER ====== %d\n",hash_number);

	/*Find the block where bucket for hash_number is saved*/
	*block_to_read = 1;
	while(count <= hash_number)
	{	
		if(BF_ReadBlock(header_info->fileDesc,*block_to_read,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		count+=(BLOCK_SIZE / sizeof(int)-1);
		if(count <= hash_number) block_to_read=(int*) block;
	}
	while(count <= hash_number)
	{
		btr++;
		count+=(BLOCK_SIZE / sizeof(int));
	}	
	if(BF_ReadBlock(header_info->fileDesc,*block_to_read,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	block+=sizeof(int);		//first int point to the next hash table block
	block+=((hash_number % (BLOCK_SIZE / sizeof(int)-1))*sizeof(int));
	bucket=(int*) block;
	over_flow_point = *bucket;
	if(*bucket == -1)
	{			
		/*Creating and Initializing block of Records*/
		if(BF_AllocateBlock(header_info->fileDesc) < 0)		
		{
			BF_PrintError("Error allocating file");
			return -1;
		}
		block_num = BF_GetBlockCounter(header_info->fileDesc)-1;
		//printf("A new block has been created with number %d\n",block_num);

		/*In First Insert ONLY set all buckets point to the same block*/
		if(init_val == 0)
		{
			block-=((hash_number % (BLOCK_SIZE / sizeof(int)-1))*sizeof(int));
			for(i=1;i<(BLOCK_SIZE / sizeof(int)-1);i++)
			{
				t=(int*) block;
				if(*t == -2)  break;
				strncpy(block,(char*)&block_num,sizeof(int));
				block+=sizeof(int);
			}
			init_val++;
		}
		else strncpy(block,(char*)&block_num,sizeof(int));
		if (BF_WriteBlock(header_info->fileDesc,*block_to_read) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
		if(BF_ReadBlock(header_info->fileDesc,block_num,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		block+=MAXREC*64*sizeof(char);
		for(i=0;i<MAXREC;i++)
		{
			strncpy(block,(char*)&zero,sizeof(int));
			block+=sizeof(int);
		}
		strncpy(block,(char*)&zero,sizeof(int));
		block+=sizeof(int);
		strncpy(block,(char*)&zero,sizeof(int));
		if (BF_WriteBlock(header_info->fileDesc,block_num) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
		/*Adding the first Record*/
		//printf("A record has been added into the block with number %d\n",block_num);
		write_record(*header_info,block_num,record,0,1);		
	}
	/*Case where bucket is not empty*/
	else
	{

		if(BF_ReadBlock(header_info->fileDesc,*bucket,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		block+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
		l = (int*)block;
		
		/*Case where block is not full*/
		if(*l<MAXREC)
		{
			block-=(sizeof(int)*MAXREC);
			val = (int*) block;
			i = 0;
			while((i<MAXREC) && (*val != 0))
			{
				block+=sizeof(int);
				val=(int*) block;
				i++;
			}
			//printf("A record has been added into the block with number %d and id %d\n",*bucket,record.id);
			write_record(*header_info,*bucket,record,i,0);	
		}
		
		/*Case where block is full*/
		else
		{

			/******Double size of hash table in blocks*******/
			block+=sizeof(int);
			local_depth = (int*)block;
			/*Case where local_depth == global_depth*/
			if(header_info->depth == *local_depth)
			{
				next = 1;
				header_info->depth++;
				do
				{
					if(BF_ReadBlock(header_info->fileDesc,next,&block) <0)
					{
						BF_PrintError("Error getting block");
						return -1;
					}
					flag=(int*)block;
					if(*flag>0) next=*flag;
				}while(*flag!=-1);
		//printf("next=%d\n",next);
				int *number,bucket_to_add,notfree=0,free,bl_num;
				bucket_to_add = (int) pow(2.0,(double)(header_info->depth-1));
		//printf("btadd=%d\n",bucket_to_add);
				for(i=0;i<(BLOCK_SIZE / sizeof(int));i++)
				{
					number=(int*)block;
					if(*number != -2) notfree++;
					if(*number == -2) break;
					block+=sizeof(int);
				}
		//printf("notfree=%d\n",notfree);
				free = BLOCK_SIZE / sizeof(int) - notfree;
				if(bucket_to_add <= free)
				{
					for(i=0;i<bucket_to_add;i++)
					{
						strncpy(block,(char*)&init,sizeof(int));
						block+=sizeof(int);
					}
					if (BF_WriteBlock(header_info->fileDesc,next) < 0)
					{
						BF_PrintError("Error writing block back");
						return -1;
					}
				}
				else
				{
					for(i=0;i<free;i++)
					{
						strncpy(block,(char*)&init,sizeof(int));
						block+=sizeof(int);
					}
					bucket_to_add-=free;
					table_block_size = bucket_to_add / (BLOCK_SIZE / sizeof(int)-1);
					if(bucket_to_add % (BLOCK_SIZE / sizeof(int)-1) != 0)	table_block_size++;
					for(i=1;i<=table_block_size;i++)
					{
						if(BF_AllocateBlock(header_info->fileDesc) < 0)				
						{
							BF_PrintError("Error allocating file");
							return -1;
						}
						bl_num = BF_GetBlockCounter(header_info->fileDesc)-1;
						//printf("Block with number %d created\n",bl_num);
						if(i==1)		//link lists
						{
							block-=BLOCK_SIZE;
							strncpy(block,(char*)&bl_num,sizeof(int));
							if (BF_WriteBlock(header_info->fileDesc,next) < 0)
							{
								BF_PrintError("Error writing block back");
								return -1;
							}
						}
						
						if(table_block_size == 1)
						{
							if(BF_ReadBlock(header_info->fileDesc,bl_num,&block) <0)
							{
								BF_PrintError("Error getting block");
								return -1;
							}
							strncpy(block,(char*)&init,sizeof(int));
							block+=sizeof(int);
							for(j=0;j<bucket_to_add;j++)
							{
								strncpy(block,(char*)&init,sizeof(int));
								block+=sizeof(int);
							}
							for(j=bucket_to_add;j<(BLOCK_SIZE / sizeof(int));j++)
							{
								strncpy(block,(char*)&init2,sizeof(int));
								block+=sizeof(int);
							}
							bucket_to_add-=(BLOCK_SIZE / sizeof(int))-1;
						}
						else
						{
							if(i!=1)
							{
								block-=BLOCK_SIZE;
								strncpy(block,(char*)&bl_num,sizeof(int));
								if (BF_WriteBlock(header_info->fileDesc,bl_num-1) < 0)
								{
									BF_PrintError("Error writing block back");
									return -1;
								}
							}
							if(BF_ReadBlock(header_info->fileDesc,bl_num,&block) <0)
							{
								BF_PrintError("Error getting block");
								return -1;
							}
							if(i == table_block_size) strncpy(block,(char*)&init,sizeof(int));
							block+=sizeof(int);
							if(i!=table_block_size)
							{
								for(j=0;j<(BLOCK_SIZE / sizeof(int))-1;j++)
								{
									strncpy(block,(char*)&init,sizeof(int));
									block+=sizeof(int);	
								}
								bucket_to_add-=(BLOCK_SIZE / sizeof(int))-1;
							}
							else
							{
								for(j=0;j<bucket_to_add;j++)
								{
									strncpy(block,(char*)&init,sizeof(int));
									block+=sizeof(int);
								}
								for(j=bucket_to_add;j<(BLOCK_SIZE / sizeof(int));j++)
								{
									strncpy(block,(char*)&init2,sizeof(int));
									block+=sizeof(int);
								}
							}
						}

						if (BF_WriteBlock(header_info->fileDesc,bl_num) < 0)
						{
							BF_PrintError("Error writing block back");
							return -1;
						}
					}

				
				}
				/*Reform Hash Table*/
				int temp_table[(int) pow(2.0,(double)(header_info->depth))];
				
				next = 1;
				int table_count=0,pr;
				void *temp2_block;
				do{
					if(BF_ReadBlock(header_info->fileDesc,next,&block) <0)
					{
						BF_PrintError("Error getting block");
						return -1;
					}
					flag=(int*)block;
					if(*flag>0) next=*flag;
					block+=sizeof(int);
					for(i=0;i<(BLOCK_SIZE / sizeof(int))-1;i++)
					{
						if (table_count == (int) pow(2.0,(double)(header_info->depth))) break;
						number=(int*)block;
						pr=*number;
						temp_table[table_count] = pr;
						table_count++;
						block+=sizeof(int);
					}
				}while(*flag!=-1);
				int j=0,kl;
				for(i=0;i<(int) pow(2.0,(double)(header_info->depth)-1);i++)   temp_table[i+(int) pow(2.0,(double)(header_info->depth)-1)] = temp_table[i];
								//for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i++) printf("copy table=%d %d\n",temp_table[i],i);

				for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i+=2) 
				{
					temp_table[i] = temp_table[(int) pow(2.0,(double)(header_info->depth)-1)+j];
					temp_table[i+1] = temp_table[(int) pow(2.0,(double)(header_info->depth)-1)+j];
					j++;
				}
				
				for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i+=2)
					{
						if(i == (int) pow(2.0,(double)(header_info->depth)))	break;
						kl = temp_table[i];
						if(kl == over_flow_point)					//change local_depth = global_depth
						{
							if(BF_ReadBlock(header_info->fileDesc,over_flow_point,&temp2_block) <0)
							{
								BF_PrintError("Error getting block");
								return -1;
							}
							temp2_block+=(MAXREC*64+(sizeof(int)*(MAXREC+1)))*sizeof(char);
							strncpy(temp2_block,(char*)&header_info->depth,sizeof(int));
							if (BF_WriteBlock(header_info->fileDesc,over_flow_point) < 0)
							{
								BF_PrintError("Error writing block back");
								return -1;
							}
						}
						kl = temp_table[i+1];
						if(kl == over_flow_point)					//new block for splitted bucket
						{
							if(BF_AllocateBlock(header_info->fileDesc) < 0)				
							{
								BF_PrintError("Error allocating file");
								return -1;
							}
							bl_num = BF_GetBlockCounter(header_info->fileDesc)-1;
							temp_table[i+1] = bl_num;
							if(BF_ReadBlock(header_info->fileDesc,bl_num,&temp2_block) <0)
							{
								BF_PrintError("Error getting block");
								return -1;
							}
							temp2_block+=(MAXREC*64+(sizeof(int)*(MAXREC+1)))*sizeof(char);
							strncpy(temp2_block,(char*)&header_info->depth,sizeof(int));
							if (BF_WriteBlock(header_info->fileDesc,bl_num) < 0)
							{
								BF_PrintError("Error writing block back");
								return -1;
							}
						}
					}
					
				//for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i++) printf("changed table=%d\n",temp_table[i]);
				
				/*Copy temp table back to blocks*/
				table_count=0;
				next=1;
				int prev;
				do{
					if(BF_ReadBlock(header_info->fileDesc,next,&block) <0)
					{
						BF_PrintError("Error getting block");
						return -1;
					}
					flag=(int*)block;
					prev=next;
					if(*flag>0) next=*flag;
					block+=sizeof(int);
					for(i=0;i<(BLOCK_SIZE / sizeof(int))-1;i++)
					{
						if (table_count == (int) pow(2.0,(double)(header_info->depth))) break;
						strncpy(block,(char*)&temp_table[table_count],sizeof(int));
						table_count++;
						block+=sizeof(int);
					}
					if (BF_WriteBlock(header_info->fileDesc,prev) < 0)
					{
						BF_PrintError("Error writing block back");
						return -1;
					}
				}while(*flag!=-1);
				
				
				for(i=0;i<MAXREC;i++)
				{
					EH_InsertEntry(header_info, getRecord(*header_info,over_flow_point,i));
				}
				EH_InsertEntry(header_info, record);
				next=1;
				
			}	
			/*Case where global_depth > local_depth*/
			else
			{
				/*Change local depth*/
				int temp_table[(int) pow(2.0,(double)(header_info->depth))];
				int table_count=0,*number,pr;
				next=1;
				do{
					if(BF_ReadBlock(header_info->fileDesc,next,&block) <0)
					{
						BF_PrintError("Error getting block");
						return -1;
					}
					flag=(int*)block;
					if(*flag>0) next=*flag;
					block+=sizeof(int);
					for(i=0;i<(BLOCK_SIZE / sizeof(int))-1;i++)
					{
						if (table_count == (int) pow(2.0,(double)(header_info->depth))) break;
						number=(int*)block;
						pr=*number;
						temp_table[table_count] = pr;
						table_count++;
						block+=sizeof(int);
					}
				}while(*flag!=-1);
				
				//for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i++) printf("copy table=%d\n",temp_table[i]);

				if(BF_ReadBlock(header_info->fileDesc,*bucket,&block) <0)
				{
					BF_PrintError("Error getting block");
					return -1;
				}
				block+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
				block+=sizeof(int);
				int loc_dep,sum=0,point=0,bl_num;
				loc_dep = *local_depth+1;
				strncpy(block,(char*)&loc_dep,sizeof(int));
				if (BF_WriteBlock(header_info->fileDesc,*bucket) < 0)
				{
					BF_PrintError("Error writing block back");
					return -1;
				}
				
				if(BF_AllocateBlock(header_info->fileDesc) < 0)				
				{
					BF_PrintError("Error allocating file");
					return -1;
				}
				bl_num = BF_GetBlockCounter(header_info->fileDesc)-1;
				
				if(BF_ReadBlock(header_info->fileDesc,bl_num,&block) <0)
				{
					BF_PrintError("Error getting block");
					return -1;
				}
				block+=(64*MAXREC+(MAXREC+1)*sizeof(int));
				strncpy(block,(char*)&loc_dep,sizeof(int));
				if (BF_WriteBlock(header_info->fileDesc,bl_num) < 0)
				{
					BF_PrintError("Error writing block back");
					return -1;
				}

				/*Adjust hash table values*/
				int flagg=0;
				for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i++)
				{
					if((flagg==0) && (temp_table[i] == *bucket)) {flagg=1;point=i;}
					if(temp_table[i] == *bucket) sum++;
				}
				for(i=0;i<sum/2;i++)
				{
					temp_table[i+point+sum/2] = bl_num;
				}
				table_count=0;
			//	for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i++) printf("changed table=%d\n",temp_table[i]);
				next=1;
				do{
					if(BF_ReadBlock(header_info->fileDesc,next,&block) <0)
					{
						BF_PrintError("Error getting block");
						return -1;
					}
					flag=(int*)block;
					if(*flag>0) next=*flag;
					block+=sizeof(int);
					for(i=0;i<(BLOCK_SIZE / sizeof(int))-1;i++)
					{
						if (table_count == (int) pow(2.0,(double)(header_info->depth))) break;
						strncpy(block,(char*)&temp_table[table_count],sizeof(int));
						table_count++;
						block+=sizeof(int);
					}
				}while(*flag!=-1);
				
				
				for(i=0;i<MAXREC;i++)
				{
					EH_InsertEntry(header_info, getRecord(*header_info,over_flow_point,i));
				}
				EH_InsertEntry(header_info, record);
				
				/*if(BF_ReadBlock(header_info->fileDesc,1,&block) <0)
					{
						BF_PrintError("Error getting block");
						return -1;
					}
					for(i=0;i<(BLOCK_SIZE / sizeof(int));i++)
					{bucket = (int*) block;
						printf("after rehash=%d\n",*bucket);
						block+=sizeof(int);
					}*/
		
		
			}
			
		}
			
	}
    return 0;
   
}


int EH_GetAllEntries(EH_info header_info, void *value) {
    int position,*k,*flag,*num_of_record,*bl,i,*val,next,count=0,block_to_read=0,n=1;
	char str1[15],str2[20],str3[25],*str,vall[25],*bvall;
	void *block;
	if(header_info.attrType == 'i')
	{
		val = (int*) value;
		position = hash_func(*val, header_info.depth);
		
	}
	else
	{
		strcpy(vall,(char*)value);
		position = hash_func_string(vall,header_info.depth);
	}
	while(count <= position)
	{
		block_to_read++;
		count+=(BLOCK_SIZE / sizeof(int));
	}
	for(i=0;i<block_to_read;i++)
	{
		if(BF_ReadBlock(header_info.fileDesc,n,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		k=(int*) block;
		n=*k;
	}
	count = 0;
	block+=sizeof(int);
	block+=((position % (BLOCK_SIZE / sizeof(int)-1))*sizeof(int));
	k=(int*)block;
	next=*k;

	if(*k != -1)
	{
		
				if(BF_ReadBlock(header_info.fileDesc,next,&block) <0)
				{
					BF_PrintError("Error getting block");
					return -1;
				}
				count++;
				block+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
				num_of_record=(int*)block;
				block-=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
				for(i=0;i<*num_of_record;i++)
				{
					if(strcmp(header_info.attrName,"id") == 0) 	//key is integer
					{
						bl = (int*)block;
						if(*bl == *val)
						{
							block+=sizeof(int);
							str = (char*)block;
							strncpy(str1,str,15*sizeof(char));
							str1[14]='\0';
							block+=15*sizeof(char);
							str = (char*)block;
							strncpy(str2,str,20*sizeof(char));
							str2[19]='\0';
							block+=20*sizeof(char);
							str = (char*)block;
							strncpy(str3,str,25*sizeof(char));
							str3[24]='\0';
							block+=25*sizeof(char);							
							printf("%d,%s,%s,%s\n",*bl,str1,str2,str3);
						}
						else block+=64*sizeof(char);
					}
					else 			//key is string
					{
						if(strcmp(header_info.attrName,"name") == 0)
						{
							block+=sizeof(int);
							bvall = (char*)block;
							block-=sizeof(int);
						}
						else if(strcmp(header_info.attrName,"surname") == 0)
						{
							block+=sizeof(int);
							block+=15*sizeof(char);
							bvall = (char*)block;
							block-=15*sizeof(char);
							block-=sizeof(int);
						}
						else if(strcmp(header_info.attrName,"city") == 0)
						{
							block+=sizeof(int);
							block+=15*sizeof(char);
							block+=20*sizeof(char);
							bvall = (char*)block;
							block-=20*sizeof(char);
							block-=15*sizeof(char);
							block-=sizeof(int);
						}
						if(strcmp(bvall,vall) == 0)
						{
							bl = (int*)block;
							block+=sizeof(int);
							str = (char*)block;
							strncpy(str1,str,15*sizeof(char));
							str1[14]='\0';
							block+=15*sizeof(char);
							str = (char*)block;
							strncpy(str2,str,20*sizeof(char));
							str2[19]='\0';
							block+=20*sizeof(char);
							str = (char*)block;
							strncpy(str3,str,25*sizeof(char));
							str3[24]='\0';
							block+=25*sizeof(char);							
							printf("%d,%s,%s,%s\n",*bl,str1,str2,str3);
						}
						else block+=64*sizeof(char);
					}
						
					
				}
				
			
	}
    else
	{
		printf("key not exists\n");
		return -1;
	}
	if(BF_ReadBlock(header_info.fileDesc,0,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	block += sizeof(char);
	block += header_info.attrLength*sizeof(char);
	block += sizeof(int);
	strncpy(block,(char*)&header_info.depth,sizeof(int));
	if (BF_WriteBlock(header_info.fileDesc,0) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	return count;
   
}


int HashStatistics(char* filename) {
    
	
	if(strcmp(filename,"EH_HASH")==0)
	{
		int next,*flag,i,pr,table_count=0,*number,bucket,block_count=0,*num_of_record,max=0,min=1000,count=1,bucket_records=0;
		void *block;
		EH_info *header_info;
		if ((header_info = EH_OpenIndex(filename)) == NULL) {
		fprintf(stderr, "Error opening hash index.\n");
		EH_CloseIndex(header_info);
		}
		int temp_table[(int) pow(2.0,(double)(header_info->depth))];
		next=1;
		do{
				if(BF_ReadBlock(header_info->fileDesc,next,&block) <0)
				{
					BF_PrintError("Error getting block");
					return -1;
				}
				flag=(int*)block;
				if(*flag>0) next=*flag;
				block+=sizeof(int);
				for(i=0;i<(BLOCK_SIZE / sizeof(int))-1;i++)
				{
					if (table_count == (int) pow(2.0,(double)(header_info->depth))) break;
					number=(int*)block;
					pr=*number;
					temp_table[table_count] = pr;
					table_count++;
					block+=sizeof(int);
				}
		}while(*flag!=-1);	
		//printf("table status\n");
		//for(i=0;i<(int) pow(2.0,(double)(header_info->depth));i++) printf("%d\n",temp_table[i]);
		i=0;
		while(i!=(int) pow(2.0,(double)(header_info->depth)))
		{
			if(BF_ReadBlock(header_info->fileDesc,temp_table[i],&block) <0)
			{
				BF_PrintError("Error getting block");
				return -1;
			}
			block+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
			num_of_record = (int*) block;
			bucket_records+=*num_of_record;
			if(*num_of_record > max) max=*num_of_record;
			if(*num_of_record < min)	min=*num_of_record;
			while(temp_table[i] == temp_table[i+1])		i++;
			i++;
			block_count++;
		}
		count = block_count;
		block_count++;
		if(((int) pow(2.0,(double)(header_info->depth))) < (BLOCK_SIZE / sizeof(int))) block_count++;
		else block_count+=(int) pow(2.0,(double)(header_info->depth)) / (BLOCK_SIZE / sizeof(int));
		printf("\nHash Statistics Results\n");
		printf("Block sum = %d\n",block_count);
		printf("Max record number bucket = %d\n",max);
		printf("Min record number bucket = %d\n",min);
		printf("Average record number bucket = %f\n",(float) bucket_records/count);
		printf("--------------------------------------\n");
		EH_CloseIndex(header_info);
	}
    return 0;
    
   
}

unsigned hash_func(int value,int end)
{
	int i=0,temp=value;
	while((value / 2) != 0)
	{
		i++;
		value = value / 2;
	}
	i++;
	if(i>=end)	return (temp >> (i-end));
	else	return temp;
}

unsigned hash_func_string(char *value,int end)
{

	int val=0,size = strlen(value);
	int i=0,temp;
	for(i=0;i<size-1;i++)	val+=value[i];
	temp=val;	
	while((val / 2) != 0)
	{
		i++;
		val = val / 2;
	}
	i++;
	if(i>=end) return (temp >> (i-end));
	else	return temp;
}

int blockFreeSpace(int block_num,EH_info* header_info)		//return 1 if block with #block_num has space for one record
{
	void *block;
	int *rec_num;
	
	if(BF_ReadBlock(header_info->fileDesc,block_num,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	block+=MAXREC*64+MAXREC*sizeof(int);
	rec_num = (int*) block;
	if(*rec_num < MAXREC)	return 1;
	else return 0;
}

int write_record(EH_info header_info,int bl_number,Record record,int position,int flag_init)
{
	int i,str_len,line,*num;
	void *block;
	char ch='0',ch2='\0';
	int one=1;
	line = (position)*64;
		if(BF_ReadBlock(header_info.fileDesc,bl_number,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		block+=line;
		strncpy(block,(char*)&record.id,sizeof(int));
		block+=sizeof(int);
		str_len = strlen(record.name);
		strncpy(block,(char*)&record.name,15*sizeof(char));
		if(str_len < 15)
		{
			block += str_len*sizeof(char);
			for(i=str_len;i<15;i++)	
			{
				if(i==str_len) strncpy(block,(char*)&ch2,sizeof(char));
				else strncpy(block,(char*)&ch,sizeof(char));
				block+=sizeof(char);
			}			
		}
		else	block += 15*sizeof(char);
		str_len = strlen(record.surname);
		strncpy(block,(char*)&record.surname,20*sizeof(char));
		if(str_len < 20)
		{
			block += str_len*sizeof(char);
			for(i=str_len;i<20;i++)	
			{
				if(i==str_len) strncpy(block,(char*)&ch2,sizeof(char));
				else strncpy(block,(char*)&ch,sizeof(char));
				block+=sizeof(char);
			}
		}
		else	block += 20*sizeof(char);
		
		str_len = strlen(record.city);
		strncpy(block,(char*)&record.city,25*sizeof(char));
		if(str_len < 25)
		{
			block += str_len*sizeof(char);
			for(i=str_len;i<25;i++)	
			{
				if(i==str_len) strncpy(block,(char*)&ch2,sizeof(char));
				else strncpy(block,(char*)&ch,sizeof(char));
				block+=sizeof(char);
			}
		}
		else	block += 25*sizeof(char);	
		
		if(BF_ReadBlock(header_info.fileDesc,bl_number,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		block+=(MAXREC*64+(position)*sizeof(int));
		strncpy(block,(char*)&one,sizeof(int));
		block+=((MAXREC)-position)*sizeof(int);
		num=(int*)block;
		int nn=*num;
		nn++;
		strncpy(block,(char*)&nn,sizeof(int));
		block+=sizeof(int);
		if(flag_init == 1)	strncpy(block,(char*)&header_info.depth,sizeof(int));		//case of block_init set local_depth = global_depth

		if (BF_WriteBlock(header_info.fileDesc,bl_number) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
		
}

Record getRecord(EH_info header_info,int bl_number,int position)
{
	void *block;
	int line = (position)*64,zero=0,*num,*id;
	char *str;
	Record rec;
	if(BF_ReadBlock(header_info.fileDesc,bl_number,&block) <0)
	{
		BF_PrintError("Error getting block");
		return ;
	}
	/*change block infos*/
	block+=(MAXREC*64+(position)*sizeof(int));
	strncpy(block,(char*)&zero,sizeof(int));
	block+=((MAXREC)-position)*sizeof(int);
	num=(int*)block;
	int nn=*num;
	nn--;
	strncpy(block,(char*)&nn,sizeof(int));
	block-=((MAXREC)-position)*sizeof(int);
	block-=(MAXREC*64+(position)*sizeof(int));
	
	/*get record values*/
	block+=line;
	id=(int*) block;
	rec.id = *id;
	block+=sizeof(int);
	str=(char*) block;
	strncpy(rec.name,(char*)str,15*sizeof(char));
	block+=15*sizeof(char);
	str=(char*) block;
	strncpy(rec.surname,(char*)str,20*sizeof(char));
	block+=20*sizeof(char);
	str=(char*) block;
	strncpy(rec.city,(char*)str,25*sizeof(char));
	block+=25*sizeof(char);
	
	return rec;
}


