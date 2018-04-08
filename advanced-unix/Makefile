# gun makefile
CC=gcc

DIR_SRCS=src
SRCS=$(shell cd $(DIR_SRCS);ls *.c)
SRCS_PATH=$(addprefix $(DIR_SRCS)/,$(SRCS))
	
all:
	$(CC) $(SRCS_PATH) main.c -o main -lpthread
	chmod +x main
