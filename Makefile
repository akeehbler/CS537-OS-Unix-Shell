# Usage - make

# Variables
CC = gcc
FILES = mysh.c alias.c 

# Compiler Flags
CFLAGS = -Wall -Werror -g

mysh : mysh.c
	$(CC) $(CFLAGS) -o mysh $(FILES)

.PHONY: clean
clean :
	rm -rf mysh *.o  
