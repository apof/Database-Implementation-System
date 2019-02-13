#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "BF.h"
#include <assert.h>



int main(int argc,char *argv[])
{
	int i,hash_table_size,rec_number=0;
	int *val=malloc(sizeof(int));
	char *vall=malloc(25*sizeof(char));
	size_t len = 0;
	ssize_t read;
	char *line = NULL,*filename;
	FILE *stream;
	Record record;

	
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
	/*Read number of records from File*/
	if((stream = fopen(filename,"r")) == NULL) {printf("File can't open\n");return -1;}
	while(read = getline(&line,&len,stream) != -1)	rec_number++;
	printf("rec_number = %d\n",rec_number);

	hash_table_size = rec_number/0.8;
	printf("Hash table size = %d\n",hash_table_size);
	//hash_table_size=1000;
	fseek(stream,0,SEEK_SET);
	
	HT_info *info;
	BF_Init();
	i=HT_CreateIndex("HT_HASH",'c',"name",10,hash_table_size+1);
	info = HT_OpenIndex("HT_HASH");
	//Info_print(info);

	/*Read input records*/
	while ((read = getline(&line, &len, stream)) != -1) {
        line[read - 2] = 0;
        char *pch;

        pch = strtok(line, ",");
        record.id = atoi(pch);

        pch = strtok(NULL, ",");
        pch++;
        pch[strlen(pch) - 1] = 0;
        strncpy(record.name, pch, sizeof(record.name));

        pch = strtok(NULL, ",");
        pch++;
        pch[strlen(pch) - 1] = 0;
        strncpy(record.surname, pch, sizeof(record.surname));

        pch = strtok(NULL, ",");
        pch++;
        pch[strlen(pch) - 1] = 0;
        strncpy(record.city, pch, sizeof(record.city));
        assert(!HT_InsertEntry(*info, record));

    }
	
	
*val=8024618;
strcpy(vall,"Demetrius");
printf("Records Found:\n");
printf("Blocks read = %d\n",HT_GetAllEntries(*info, vall));
*val=4005068;
strcpy(vall,"Czolba");
//printf("Blocks read = %d\n",HT_GetAllEntries(*info, vall));
*val=12051134;
strcpy(vall,"Westbrooke");
//printf("Blocks read = %d\n",HT_GetAllEntries(*info, vall));
	fclose(stream);
	free(filename);
	free(val);
	free(vall);
	HT_CloseIndex(info);

	HashStatistics("HT_HASH");


}
