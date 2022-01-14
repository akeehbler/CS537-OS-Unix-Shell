// Copyright 2021 Alec Keehbler
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct node_c {
  struct node_c *next;
  char* alias;
  char* command;
};

int add_alias(char* name, char* command);

int delete_alias(char* name);

char* decode_alias(char* name);

void print_aliases(void);

void free_aliases(void);
