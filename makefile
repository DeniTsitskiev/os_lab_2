# Простой Makefile для сборки проекта

CC = gcc
CFLAGS = -Wall

all: generate_matrix main

generate_matrix: generate_matrix.c
	$(CC) -o generate_matrix generate_matrix.c -lm

main: main.c
	$(CC) -o main main.c -pthread -lm

clean:
	rm -f generate_matrix main *.txt

test: generate_matrix main
	./generate_matrix 4 test_system.txt
	./main test_system.txt 2

.PHONY: all clean test