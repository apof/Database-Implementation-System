.PHONY: sort clean

sort: main.o sort.o func.o quicksort.o BF_32.a
	gcc -o sort main.o sort.o func.o quicksort.o BF_32.a -lm
	rm *.o
main.o: main.c 
	gcc -c main.c
sort.o: sort.h sort.c
	gcc -c sort.c
quicksort.o: quicksort.h quicksort.c
	gcc -c quicksort.c
func.o: func.h func.c
	gcc -c func.c

clean:
	rm sort