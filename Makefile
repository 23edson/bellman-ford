#
# Roteamento  Bellman-Ford
#
# Aluno: Edson Lemes da Silva
#
# Makefile ---

all: prog

prog: *.c *.h
	gcc funcs.h readFiles.c router.c -D_REENTRANT -lpthread -o main -Wall
