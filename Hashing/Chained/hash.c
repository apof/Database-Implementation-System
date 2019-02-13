#include "BF.h"
#include "hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if BLOCK_SIZE == 512
#define DEC 1
#endif
#if BLOCK_SIZE == 1024
#define DEC 2
#endif
#define MAXREC ((BLOCK_SIZE / 64) - DEC)


void Info_print(HT_info* info){
	printf("fileDesc = %d\n",info->fileDesc);
	printf("attrType = %c\n", info->attrType);
	printf("attrName = %s\n", info->attrName);
	printf("attrLength = %d\n", info->attrLength);
	printf("numBuckets = %ld\n", info->numBuckets);}
	
	
int HT_CreateIndex( char *fileName, char attrType, char* attrName, int attrLength, int buckets) {
	int fileDesc,str_len,i,j,table_block_size;
	int init = -1,zero=0;;
	void *block;
	char ch='0',ch2='\0';
	table_block_size = buckets / (BLOCK_SIZE / sizeof(int));
	if(buckets % (BLOCK_SIZE / sizeof(int)) != 0)	table_block_size++;
	
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
	strncpy(block,(char*)&buckets,sizeof(long int));
	
	if (BF_WriteBlock(fileDesc,0) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	
	for(i=1;i<=table_block_size;i++)
	{
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
		if(i*(BLOCK_SIZE / sizeof(int)) < buckets)
		{
			for(j=0;j<(BLOCK_SIZE / sizeof(int));j++)	
			{
				strncpy(block,(char*)&init,sizeof(int));
				block+=sizeof(int);
			}
		}
		else
		{
			for(j=0;j<buckets-(i-1)*(BLOCK_SIZE / sizeof(int));j++)	
			{
				strncpy(block,(char*)&init,sizeof(int));
				block+=sizeof(int);
			}
			for(j=buckets-(i-1)*(BLOCK_SIZE / sizeof(int));j<(BLOCK_SIZE / sizeof(int));j++)	
			{
				strncpy(block,(char*)&zero,sizeof(int));
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


HT_info* HT_OpenIndex(char *fileName) {
    int fileDesc;
	void *block;
	HT_info *info = malloc(sizeof(HT_info));
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
	info->numBuckets = (long int) *k;
    return info;
    
} 


int HT_CloseIndex( HT_info* header_info ) {
    
	if (BF_CloseFile(header_info->fileDesc) < 0) {
			BF_PrintError("Error closing file");
			return -1;
		}
	free(header_info->attrName);
	free(header_info);
    
    return 0;
    
}

int HT_InsertEntry(HT_info header_info, Record record) {
    
	int block_num,str_len,one=1,zero=0,i,hash_number,block_to_read=0,count=0;
	void *block;
	int *k,*l,*flag,next;
	char ch='0';
	if(header_info.attrType == 'i')	hash_number = hash_func(record.id,header_info.numBuckets);		//key is integer
	else if(strcmp(header_info.attrName,"name") == 0)	hash_number = hash_func_string(record.name,header_info.numBuckets);
	else if(strcmp(header_info.attrName,"surname") == 0)	hash_number = hash_func_string(record.surname,header_info.numBuckets);
	else if(strcmp(header_info.attrName,"city") == 0)	hash_number = hash_func_string(record.city,header_info.numBuckets);
	else{printf("there is no such key\n");return -1;}

	while(count <= hash_number)
	{
		block_to_read++;
		count+=(BLOCK_SIZE / sizeof(int));
	}
	
	if(BF_ReadBlock(header_info.fileDesc,block_to_read,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	block+=((hash_number % (BLOCK_SIZE / sizeof(int)))*sizeof(int));
	k=(int*)block;
	next=*k;

	/*Case where this bucket is empty*/
	if(*k == -1)
	{			
		/*Creating and Initializing block of Records*/
		if(BF_AllocateBlock(header_info.fileDesc) < 0)		
		{
			BF_PrintError("Error allocating file");
			return -1;
		}
		block_num = BF_GetBlockCounter(header_info.fileDesc)-1;
		//printf("A new block has been created with number %d\n",block_num);
		/*if(BF_ReadBlock(header_info.fileDesc,1,&block) <0)
		{
			BF_PrintError("Error getting block");
		}*/
		strncpy(block,(char*)&block_num,sizeof(int));
		if (BF_WriteBlock(header_info.fileDesc,block_to_read) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
		if(BF_ReadBlock(header_info.fileDesc,block_num,&block) <0)
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
		if (BF_WriteBlock(header_info.fileDesc,block_num) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
		/*Adding the first Record*/
		//printf("A record has been added into the block with number %d\n",block_num);
		write_record(header_info,block_num,record,0);	
	}
	/*Case where bucket is not empty*/
	else
	{
		if(BF_ReadBlock(header_info.fileDesc,*k,&block) <0)
		{
			BF_PrintError("Error getting block");
			return -1;
		}
		block+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
		l = (int*)block;
		/*Case where block is not full*/

		if(*l<MAXREC)
		{
			//printf("A record has been added into the block with number %d\n",*k);
			write_record(header_info,*k,record,*l);	
		}
		/*Case where block is full*/
		else
		{
			do{
				if(BF_ReadBlock(header_info.fileDesc,next,&block) <0)
				{
					BF_PrintError("Error getting block");
					return -1;
				}
				block+=(MAXREC*64+(sizeof(int)*MAXREC+sizeof(int)))*sizeof(char);
				flag=(int*)block;
				if(*flag>0) next=*flag;
			}while(*flag!=0);
			block-=sizeof(int);
			l=(int*)block;
			/*Case where there is free space into a next block in list*/
			if(*l<MAXREC)	
			{
				//printf("A record has been added into the block with number %d\n",next);
				write_record(header_info,next,record,*l);
			}
			else
			{
				if(BF_AllocateBlock(header_info.fileDesc) < 0)		
				{
					BF_PrintError("Error allocating file");
					return -1;
				}
				block_num = BF_GetBlockCounter(header_info.fileDesc)-1;
				//printf("A new block has been created in list with number %d\n",block_num);
				block+=sizeof(int);
				strncpy(block,(char*)&block_num,sizeof(int));
				if (BF_WriteBlock(header_info.fileDesc,next) < 0)
				{
					BF_PrintError("Error writing block back");
					return -1;
				}
				
				if(BF_ReadBlock(header_info.fileDesc,block_num,&block) <0)
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
				if (BF_WriteBlock(header_info.fileDesc,block_num) < 0)
				{
					BF_PrintError("Error writing block back");
					return -1;
				}
				//printf("A record has been added into the block with number %d\n",block_num);
				write_record(header_info,block_num,record,0);
			}
		}
	}
	
		
    
    return 0;
    
}



int HT_GetAllEntries(HT_info header_info, void *value) {
    
	int position,*k,*flag,*num_of_record,*bl,i,*val,next,count=0,block_to_read=0,str_len;
	char str1[15],str2[20],str3[25],*str,vall[25],*bvall;
	void *block;
	char ch='0',ch2='\0';
	if(header_info.attrType == 'i')
	{
		val = (int*) value;
		position = hash_func(*val, header_info.numBuckets);
		
	}
	else
	{
		strcpy(vall,(char*)value);
		position = hash_func_string(vall,header_info.numBuckets);
	}
	
	while(count <= position)
	{
		block_to_read++;
		count+=(BLOCK_SIZE / sizeof(int));
	}
	count = 0;
	if(BF_ReadBlock(header_info.fileDesc,block_to_read,&block) <0)
	{
		BF_PrintError("Error getting block");
		return -1;
	}
	block+=((position % (BLOCK_SIZE / sizeof(int)))*sizeof(int));
	k=(int*)block;
	next=*k;
	

	if(*k != -1)
	{
		do{
				if(BF_ReadBlock(header_info.fileDesc,next,&block) <0)
				{
					BF_PrintError("Error getting block");
					return -1;
				}
				count++;
				block+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
				num_of_record=(int*)block;
				block+=sizeof(int);
				flag=(int*)block;
				block-=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
				block-=sizeof(int);
				
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
				if(*flag>0) next=*flag;
			}while(*flag!=0);
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
	strncpy(block,&header_info.attrType,sizeof(char));
	block += sizeof(char);
	str_len = strlen(header_info.attrName);
	strncpy(block,header_info.attrName,header_info.attrLength*sizeof(char));
	if(str_len < header_info.attrLength)
	{
		block += str_len*sizeof(char);
		for(i=str_len;i<header_info.attrLength;i++)	
		{
			if(i==str_len) strncpy(block,(char*)&ch2,sizeof(char));
			else strncpy(block,(char*)&ch,sizeof(char));
			block+=sizeof(char);
		}
	}
	else	block += header_info.attrLength*sizeof(char);
	strncpy(block,(char*)&header_info.attrLength,sizeof(int));
	block += sizeof(int);
	strncpy(block,(char*)&header_info.numBuckets,sizeof(long int));
	
	if (BF_WriteBlock(header_info.fileDesc,0) < 0)
	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	return count;

}


int HashStatistics(char* filename) {
    
	int count=0,table_block_size=1,i,next,*k,j,*flag,min=10000,max=0,count2=0,bucket_records=0,bucket_no=0,*num_of_record,count3=0,count4=0;
	void *block,*block2;
	int total=0;
	
	if(strcmp(filename,"HT_HASH")==0)
	{
		HT_info *header_info;
		//header_info = HT_OpenIndex(filename);
		if ((header_info = HT_OpenIndex(filename)) == NULL) {
		fprintf(stderr, "Error opening hash index.\n");
		HT_CloseIndex(header_info);
		}
		table_block_size = header_info->numBuckets / (BLOCK_SIZE / sizeof(int));
		if(header_info->numBuckets% (BLOCK_SIZE / sizeof(int)) != 0)	table_block_size++;
		//printf("table=%d\n",table_block_size);
		count+=table_block_size;
		for(i=1;i<=table_block_size;i++)
		{
			if(BF_ReadBlock(header_info->fileDesc,i,&block) <0)
			{
				BF_PrintError("Error getting block");
				return -1;
			}
			for(j=0;j<(BLOCK_SIZE/sizeof(int));j++)
			{
				total++;
				bucket_no++;
				k=(int*)block;
				next=*k;
				block+=sizeof(int);
				if((*k==0) || (*k>640000))	break;
				//printf("next out=%d\n",*k);
				if((*k!=-1))
				{
					count2 = 0;
					bucket_records = 0;
					do
					{
						if(BF_ReadBlock(header_info->fileDesc,next,&block2) <0)
						{
							BF_PrintError("Error getting block");
							return -1;
						}
						count++;	
						block2+=(MAXREC*64+(sizeof(int)*MAXREC))*sizeof(char);
						num_of_record=(int*)block2;
						bucket_records+=*num_of_record;
						if(*num_of_record > max) max=*num_of_record;
						if(*num_of_record < min)	min=*num_of_record;
						block2+=sizeof(int);
						flag=(int*)block2;
						if(*flag>0) {next=*flag;count3++;count2++;}
					}while(*flag!=0);
					
					if(count2>0)	count4++;


				}

			}
			
		}
		printf("\nHash Statistics Results\n");
		printf("Block sum = %d\n",count);
		printf("Max record number bucket = %d\n",max);
		printf("Min record number bucket = %d\n",min);
		printf("Average record number bucket = %f\n",(float) bucket_records/(int)header_info->numBuckets);
		printf("--------------------------------------\n");
		printf("Average Block in buckets = %f\n",(float) (count-1-table_block_size)/(int)header_info->numBuckets);
		printf("Bucket with overflow = %d\n",count4);		
		printf("Block with overflow = %d\n",count3);
		HT_CloseIndex(header_info);

	}	
    return 0;
    
}

int hash_func(int key, int nbuckets)
{
	return (key) % nbuckets;
}

int hash_func_string(char *str,int nbuckets)
{
int val=0,size = strlen(str);
	int i=0,temp;
	for(i=0;i<size-1;i++)	val+=str[i];
	temp=val;
	return val % nbuckets;
}

/*{
	unsigned long hash = 5381;
    int c;
    while (c = *str++)       hash = ((hash << 5) + hash) + c; 
	return hash;
}*/


int write_record(HT_info header_info,int bl_number,Record record,int position)
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
		//(*num)++;printf("ok\n");
		strncpy(block,(char*)&nn,sizeof(int));

		if (BF_WriteBlock(header_info.fileDesc,bl_number) < 0)
		{
			BF_PrintError("Error writing block back");
			return -1;
		}
		
}
