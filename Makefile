functions.o: lib.s
	as -o functions.o lib.s

main.o: pong.c
	gcc -c -o main.o pong.c -std=c99 

main: main.o functions.o
	gcc -g -o main main.o functions.o	

run: main
	./main

teste: main
	gdb main