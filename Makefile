CC = gcc

all: ifj2019

ifj2019: *.c
	${CC} $^ -o $@

clean:
	rm ifj2019
