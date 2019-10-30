# Implementation of imperative language IFJ2019 translator
# Makefile

CC = gcc
FILES = ifj2019.c scanner.c dynamic-string.c dynamic-stack.c symtable.c dynamic-symstack.c
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic

all: ifj2019

ifj2019: $(FILES)
	$(CC) $(CFLAGS) -o $@ $(FILES)

clean:
	rm *.o ifj2019
