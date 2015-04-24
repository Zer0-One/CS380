#
# Makefile for project1. 
#
# Compiler: gcc

cc = gcc
server_sources = src/echod/*.c
client_sources = src/echo_client/*.c
bin = bin

all : clean project1
all-debug : clean project1-debug

project1 :
			mkdir bin
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 $(server_sources) -o $(bin)/echod
			$(cc) -O2 -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 $(client_sources) -o $(bin)/echo_client
			chmod +x $(bin)/*

project1-debug :
			mkdir bin
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 $(serer_sources) -o $(bin)/echod_debug
			$(cc) -g -std=c99 -pedantic-errors -Wall -D_XOPEN_SOURCE=600 $(client_sources) -o $(bin)/echo_client_debug
			chmod +x $(bin)/*

clean :
			-rm -r bin
