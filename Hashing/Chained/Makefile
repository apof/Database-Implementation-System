.PHONY: hash clean

hash: main.o hash.o BF_32.a
	gcc -o hash main.o hash.o BF_32.a
	rm *.o
main.o: main.c 
	gcc -c main.c
hash.o: hash.h hash.c
	gcc -c hash.c

clean:
	rm hash