# ==========================================================
# ICT374 Assignment 2
# Makefile for building the Unix Shell
#
# Builds:
#   shell
#
# Removes:
#   object files and executable using 'make clean'
# ==========================================================

CC=gcc

CFLAGS=-Wall -Wextra -std=c11 -g

OBJS=main.o terminal.o parser.o execute.o builtin.o signals.o history.o

shell: $(OBJS)
	$(CC) $(CFLAGS) -o shell $(OBJS)

main.o: main.c shell.h terminal.h parser.h execute.h builtin.h signals.h history.h
	$(CC) $(CFLAGS) -c main.c

terminal.o: terminal.c terminal.h shell.h history.h builtin.h
	$(CC) $(CFLAGS) -c terminal.c

parser.o: parser.c parser.h shell.h
	$(CC) $(CFLAGS) -c parser.c

execute.o: execute.c execute.h shell.h signals.h parser.h
	$(CC) $(CFLAGS) -c execute.c

builtin.o: builtin.c builtin.h shell.h history.h
	$(CC) $(CFLAGS) -c builtin.c

signals.o: signals.c signals.h
	$(CC) $(CFLAGS) -c signals.c

history.o: history.c history.h shell.h
	$(CC) $(CFLAGS) -c history.c

clean:
	rm -f *.o shell
