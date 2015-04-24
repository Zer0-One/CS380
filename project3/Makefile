#
# Makefile for project3
#
# Compiler: gcc

cc = gcc
sources = src/ipv4/*.c
bin = bin

all : clean project3
all-debug : clean project3-debug

project3 :
			mkdir bin
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/ipv4
			chmod +x $(bin)/*

project3-debug :
			mkdir bin
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/ipv4_debug
			chmod +x $(bin)/*

clean :
			-rm -r bin
