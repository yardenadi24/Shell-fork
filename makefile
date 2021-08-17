all: task3 LineParser linker

task3: task3.c
	gcc -g -m32 -c -o task3.o task3.c

LineParser: LineParser.c
	gcc -g -m32 -c -o LineParser.o LineParser.c

linker: task3.o LineParser.o
	gcc -g -m32 task3.o LineParser.o -o myshell
	rm task3.o LineParser.o	

.PHONY: clean
clean:
	rm -rf ./*.o myshell	