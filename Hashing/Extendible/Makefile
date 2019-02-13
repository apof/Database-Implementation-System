.PHONY: hash clean

exhash: main.o exhash.o BF_32.a 
	gcc -o exhash main.o exhash.o BF_32.a -lm
	rm *.o
main.o: main.c 
	gcc -c main.c
exhash.o: exhash.h exhash.c
	gcc -c exhash.c

clean:
	rm exhash