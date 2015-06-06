#
# Makefile for project6
#
# Compiler: gcc

cc = gcc
sources = src/tcp/*.c
bin = bin

all : clean project6
all-debug : clean project6-debug

project6 :
			mkdir bin
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_DEFAULT_SOURCE=1 -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/tcp
			chmod +x $(bin)/*

project6-debug :
			mkdir bin
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_DEFAULT_SOURCE=1 -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/tcp_debug
			chmod +x $(bin)/*

clean :
			-rm -r bin
