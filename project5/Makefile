#
# Makefile for project5
#
# Compiler: gcc

cc = gcc
sources = src/udp/*.c
bin = bin

all : clean project5
all-debug : clean project5-debug

project5 :
			mkdir bin
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/udp
			chmod +x $(bin)/*

project5-debug :
			mkdir bin
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 -lm $(sources) -o $(bin)/udp_debug
			chmod +x $(bin)/*

clean :
			-rm -r bin
