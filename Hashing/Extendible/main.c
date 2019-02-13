#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exhash.h"
#include "BF.h"
#include <assert.h>
#include <time.h>


int main(int argc, char *argv[])
{
	srand (time(NULL));
	int i,*val=malloc(sizeof(int));
	Record record;
	char *vall=malloc(25*sizeof(char));
	size_t len = 0;
	ssize_t read;
	char *line = NULL,*filename;
	FILE *stream;
	
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
		
	EH_info *info;
	BF_Init();
	if(EH_CreateIndex("EH_HASH","id",'i',10,0) != 0) {printf("Error in EH_CreateIndex\n");return -1;}
	if((info = EH_OpenIndex("EH_HASH")) == NULL) {printf("Error in EH_OpenIndex\n");return -1;}
	int coo=0;
	while ((read = getline(&line, &len, stream)) != -1) {
		coo++;
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

        assert(!EH_InsertEntry(info, record));

    }
	*val=12051134;
	strcpy(vall,"Demetrius");
	printf("Records Found:\n");
	printf("Blocks read = %d\n",EH_GetAllEntries(*info, val));
	
	
	if(EH_CloseIndex(info) != 0) {printf("Error in EH_CloseIndex\n");return -1;};
	HashStatistics("EH_HASH");
	
	return 0;
}
