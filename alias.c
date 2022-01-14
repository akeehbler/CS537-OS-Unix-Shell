// Copyright 2021 Alec Keehbler
#include "alias.h"

/*
 * Global variables for tracking the head and the current node that is being
 * used
 */
struct node_c *head = NULL;
struct node_c *curr_node = NULL;

/*
 * This function adds an alias node to the linked list with command being what
 * will be run if the user types the name.
 */
int add_alias(char *name, char *command) {
  // Check to make sure that the name isn't alias, unalias, or exit
  if (strcmp(name, "alias") == 0 || strcmp(name, "unalias") == 0 ||
      strcmp(name, "exit") == 0) {
    write(STDERR_FILENO, "alias: Too dangerous to alias that.\n", 36);
    return -1;
  }
  struct node_c *alias_node = (struct node_c *)malloc(sizeof(struct node_c));
  alias_node->next = NULL;
  alias_node->command = command;
  alias_node->alias = name;
  // If there is no head yet then set the head
  if (head == NULL) {
    head = alias_node;
  } else {
    // Go through the linked list and append this to the last spot
    curr_node = head;
    while (curr_node->next != NULL) {
      curr_node = curr_node->next;
    }
    curr_node->next = alias_node;
  }
  return 1;
}

/*
* Checks if the given name is in the Linked List, and if it is
* then return the command associated with it
*/
char *decode_alias(char *alias_name) {
  // Check to make sure there are aliases in the list
  if (head == NULL) {
    return NULL;
  }
  curr_node = head;
  // Iterate through the linked list looking for the alias
  while (strcmp(curr_node->alias, alias_name) != 0) {
    if (curr_node->next != NULL) {
      curr_node = curr_node->next;
    } else {
      // Went through whole list and didn't find the match
      return NULL;
    }
  }
  return curr_node->command;
}

/*
* Delete the given alias name if it exists within the Linked List
*/
int delete_alias(char *alias_name) {
  if (head == NULL) {
    return -1;
  }
  curr_node = head;
  struct node_c *prev_node = NULL;
  // Loops through to find the matching alias in the list
  // Returns -1 if it was not found
  while (strcmp(curr_node->alias, alias_name) != 0) {
    if (curr_node->next != NULL) {
      prev_node = curr_node;
      curr_node = curr_node->next;
    } else {
      return -1;
    }
  }
  // Delete the node and add the proper links in
  if (curr_node == head) {
    struct node_c *prev_head = head;
    head = head->next;
    free(prev_head);
  } else {
    // Doesn't look like this free is actually helping mem leaks
    struct node_c *free_this = prev_node->next;
    prev_node->next = curr_node->next;
    free(free_this);
  }
  return 1;
}

/*
* Print all the nodes in the Linked List
*/
void print_aliases() {
  curr_node = head;
  while (curr_node != NULL) {
    printf("%s %s\n", curr_node->alias, curr_node->command);
    fflush(stdout);
    curr_node = curr_node->next;
  }
}

/*
* Frees all the nodes in the linked list that were malloced
*/
void free_aliases() {
  curr_node = head;
  while (curr_node->next != NULL) {
    struct node_c *next_node = curr_node->next;
    free(curr_node);
    curr_node = next_node;
  }
  free(curr_node);
}
