#include "quicksort.h"
#include <string.h>

void swap(int num1, int num2, Record *recTable) {
   Record temp = recTable[num1];
   recTable[num1] = recTable[num2];
   recTable[num2] = temp;
}

int partition1(int left, int right, int pivot, Record *recTable) {
   int leftPointer = left -1;
   int rightPointer = right;

   while(1) {
      while(recTable[++leftPointer].id < pivot) {
         //do nothing
      }
		
      while(rightPointer > 0 && recTable[--rightPointer].id > pivot) {
         //do nothing
      }

      if(leftPointer >= rightPointer) {
         break;
      } else {
         swap(leftPointer,rightPointer,recTable);
      }
   }
	
   swap(leftPointer,right,recTable);
   return leftPointer;
}

int partition2(int left, int right, char* pivot, Record *recTable, int fieldNo) {
   int leftPointer = left -1;
   int rightPointer = right;
	if(fieldNo == 1){
		while(1) {
			while(strcmp(recTable[++leftPointer].name,pivot) < 0) {
         //do nothing
			}
		
			while(rightPointer > 0 && strcmp(recTable[--rightPointer].name,pivot) > 0) {
         //do nothing
			}

			if(leftPointer >= rightPointer) {
				break;
			} else {
				swap(leftPointer,rightPointer,recTable);
			}
		}
	}
	else if(fieldNo == 2){
		while(1) {
			while(strcmp(recTable[++leftPointer].surname,pivot) < 0) {
         //do nothing
			}
		
			while(rightPointer > 0 && strcmp(recTable[--rightPointer].surname,pivot) > 0) {
         //do nothing
			}

			if(leftPointer >= rightPointer) {
				break;
			} else {
				swap(leftPointer,rightPointer,recTable);
			}
		}
	}
	else if(fieldNo == 3){
		while(1) {
			while(strcmp(recTable[++leftPointer].city,pivot) < 0) {
         //do nothing
			}
		
			while(rightPointer > 0 && strcmp(recTable[--rightPointer].city,pivot) > 0) {
         //do nothing
			}

			if(leftPointer >= rightPointer) {
				break;
			} else {
				swap(leftPointer,rightPointer,recTable);
			}
		}
	}
	
   swap(leftPointer,right,recTable);
   return leftPointer;
}

void quickSort(int left, int right, Record *recTable, int fieldNo) {
   if(right-left <= 0) {
      return;   
   } else {
	    
	   if(fieldNo == 0)	
	   {
		   int pivot = recTable[right].id;
		   int partitionPoint = partition1(left, right, pivot, recTable);
		   quickSort(left,partitionPoint-1,recTable,fieldNo);
		   quickSort(partitionPoint+1,right,recTable,fieldNo);
	   }
	   else				
	   {
		   char pivot[25];
		   if(fieldNo == 1)	strcpy(pivot,recTable[right].name);
		   if(fieldNo == 2)	strcpy(pivot,recTable[right].surname);
		   if(fieldNo == 3)	strcpy(pivot,recTable[right].city);
	   
		   int partitionPoint = partition2(left, right, pivot, recTable, fieldNo);
		   quickSort(left,partitionPoint-1,recTable,fieldNo);
		   quickSort(partitionPoint+1,right,recTable,fieldNo);
	   }
   }        
}
