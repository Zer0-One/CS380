#
# Makefile for project2
#
# Compiler: gcc

cc = gcc
sources = src/chat/*.c
bin = bin

all : clean project2
all-debug : clean project2-debug

project2 :
			mkdir bin
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 $(sources) -o $(bin)/chat
			chmod +x $(bin)/*

project2-debug :
			mkdir bin
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 $(sources) -o $(bin)/chat_debug
			chmod +x $(bin)/*

clean :
			-rm -r bin
