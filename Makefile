all: prog

prog: *.c *.h
	gcc funcs.h readFiles.c router.c -D_REENTRANT -lpthread -o m -Wall
