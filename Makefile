all: ep1 ep1sh

ep1sh: ep1sh.c
	gcc ep1sh.c -o ep1sh -lreadline

ep1: ep1.c
	gcc ep1.c -o ep1 -lpthread -lm
