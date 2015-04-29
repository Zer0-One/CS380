#
# Makefile for project4
#
# Compiler: gcc

cc = gcc
sources = src/ipv6/*.c
bin = bin

all : clean project4
all-debug : clean project4-debug

project4 :
			mkdir bin
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/ipv6
			chmod +x $(bin)/*

project4-debug :
			mkdir bin
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/ipv6_debug
			chmod +x $(bin)/*

clean :
			-rm -r bin
