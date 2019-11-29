# Implementation of imperative language IFJ2019 compiler
# Makefile

CC=gcc
FILES=ifj2019.c scanner.c dynamic-string.c dynamic-stack.c symtable.c dynamic-symstack.c parser.c expression.c code-gen.c
HEADS=ifj2019.c scanner.h dynamic-string.h dynamic-stack.h symtable.h dynamic-symstack.h parser.h expression.h code-gen.h
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic

all: ifj2019

ifj2019: $(FILES) $(HEADS)
	$(CC) $(CFLAGS) -o $@ $(FILES)

clean:
	rm *.o ifj2019

zip:
	zip *.c *.h Makefile
