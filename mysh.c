// Copyright 2021 Alec Keehbler
#include "alias.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX_CMD_LINE 512
#define MAX_CMD_TOKEN 100

/*
* Custom Structure to hold various argument details
*/
typedef struct argument {
  char **argv;
  int argc;
  int valid;
  int redirect;
} argument_c;

/*
* Prints to stderror given a message
*/
void error_message(char *message) {
  write(STDERR_FILENO, message, strlen(message));
}

/*
* Concatenates all strings into one string and returns the combined string
*/
char *concatenate(size_t size, char **strings) {
  size_t *lengths = (size_t *)malloc(sizeof(size_t) * size);
  size_t total = 0;
  char *final_char;
  char *point;
  // Start at i = 2 since don't want first 3 arguments in argv(strings)
  for (int i = 2; i < size; i++) {
    lengths[i] = strlen(strings[i]);
    total += strlen(strings[i]);
  }
  total += size;
  final_char = point = malloc(total);
  for (int i = 2; i < size; i++) {
    strcpy(point, strings[i]);
    point += lengths[i];
    if (i < size - 1) {
      strcpy(point, " ");
      point++;
    }
  }
  free(lengths);
  return final_char;
}

/*
* Checks for redirection errors and returns -1 if there was an issue
* Returns 1 if there was no issue
*/
int redirect(char *start, char *command) {
  // If '>' character was the first character
  if (start - command == 0) {
    error_message("Redirection misformatted.\n");
    return -1;
  }
  char *last_index = strrchr(command, '>');
  // Checks if there are multiple '>' characters
  if (start - command != last_index - command) {
    error_message("Redirection misformatted.\n");
    return -1;
  }
  // Checks if '>' is the last character
  if (start - command + 1 == strlen(command)) {
    error_message("Redirection misformatted.\n");
    return -1;
  }
  return 1;
}

/**
 *
 * This function returns a struct that holds the commands (argv), the number of
 * commands (argc), if it is a redirect (redirect), and if it is valid (valid)
 *
 */
argument_c parse_command(char *command) {
  char *token;
  char **argv = (char **)malloc(sizeof(char *) * MAX_CMD_TOKEN);
  int argc = 0;
  // Use struct so we can have multiple return values
  argument_c args;
  // Default set valid to 1 (true)
  args.valid = 1;
  // Track if it is redirected (0 if not, 1 if it is, -1 if issue)
  args.redirect = 0;
  // Replace tab characters with just a space
  for (int i = 0; i < strlen(command); i++) {
    if (command[i] == '\t') {
      command[i] = ' ';
    } else if (command[i] == '\n') {
      // Replace the newline character with a terminator so it is a string
      command[i] = '\0';
    }
  }
  // Get the index of the redirect character '>' if it exists
  char *redirect_index = strchr(command, '>');
  if (redirect_index != NULL) {
    // Check to make sure the redirect doesn't have issues
    args.redirect = redirect(redirect_index, command);
  }
  // Checked for > being first or last character and if there are multiple
  if (args.redirect == -1) {
    args.valid = 0;
    return args;
  }
  // Copy the command so the original isn't modified
  char *copy_command = strdup(command);
  // Gets the first token
  token = strtok(copy_command, " ");
  // Get the rest of the tokens delimiting based on spaces
  while (token != NULL) {
    argv[argc] = strdup(token);
    token = strtok(NULL, " ");
    argc++;
  }
  // Now that we have the tokens, check if there is more than one file after >
  // if there is a >
  if (args.redirect == 1) {
    // Loop through all of the tokens
    for (int i = 0; i < argc; i++) {
      if (strstr(argv[i], ">") != NULL) {
        // Checks to see if it is only >
        if (strlen(argv[i]) == 1) {
          if (i != argc - 2) {
            error_message("Redirection misformatted.\n");
            args.valid = 0;
            free(copy_command);
            return args;
          }
          // Replace the > with the file afterit
          argv[i] = argv[i + 1];
          argc--;
        } else {
          // > is part of a larger token and must separate the token
          char *index = strchr(argv[i], '>');
          if (index - argv[i] == 0) {
            argv[i] = argv[i] + sizeof(char);
          } else if (index - argv[i] + 1 == strlen(argv[i])) {
            // It is the last part in the token
            argv[i][strlen(argv[i]) - 1] = '\0';
          } else {
            // It must be in the middle
            char *copy_token = strdup(argv[i]);
            char *prev_token = strtok(copy_token, ">");
            char *last_token = strtok(NULL, ">");
            argv[i] = prev_token;
            argv[i + 1] = last_token;
            argc++;
          }
        }
      }
    }
  }
  // Needs to be Null Terminated so adds that in
  argv[argc] = NULL;
  // Checks if there are no arguments, if not then invalid
  if (argv[0] == NULL) {
    args.valid = 0;
  }
  args.argv = argv;
  args.argc = argc;
  free(copy_command);
  return args;
}

/*
* Acutally executes the given commands
*/
void run_command(argument_c args) {
  int pid;
  // !! Free stuff
  // Exit command leaves
  if (strcmp(args.argv[0], "exit") == 0) {
    _exit(0);
  } else if (strcmp(args.argv[0], "alias") == 0) {
    // ALIAS Command: deal with it accordingly
    // If type just alias then print all the aliases
    if (args.argc == 1) {
      print_aliases();
    } else if (args.argc == 2) {
      // If type alias then a word then find if it exists and print
      char *command = decode_alias(args.argv[1]);
      char *alias_name = args.argv[1];
      if (command != NULL) {
        // print the command each token separated by a space
        printf("%s %s\n", alias_name, command);
        fflush(stdout);
      }
    } else {
      // Check if exactly 3, then need to alias 2nd arg to 3rd arg
      if (args.argc == 3) {
        // If the alias exists, delete the other one and add in the new one
        if (decode_alias(args.argv[1]) != NULL) {
          delete_alias(args.argv[1]);
          add_alias(args.argv[1], args.argv[2]);
        } else {
          // Just add in the alias if it's new
          add_alias(args.argv[1], args.argv[2]);
        }
      } else {
        // More than 3 args: concatenate all tokens except first 2
        char *command_connected = concatenate(args.argc, args.argv);
        if (decode_alias(args.argv[1]) != NULL) {
          delete_alias(args.argv[1]);
          add_alias(args.argv[1], command_connected);
        } else {
          add_alias(args.argv[1], command_connected);
        }
      }
    }
  } else if (strcmp(args.argv[0], "unalias") == 0) {
    // Unalias command
    if (args.argc == 1 || args.argc > 2) {
      error_message("unalias: Incorrect number of arguments.\n");
    } else {
      delete_alias(args.argv[1]);
    }
  } else if (strcmp(args.argv[0], "") != 0) {
    // Any other command given that isn't alias, unalias, or exit
    pid = fork();
    if (pid < 0) {
      error_message("Fork failed\n");
    } else if (pid == 0) {
      // Child Process
      if (args.redirect == 1) {
        char *filepointer = args.argv[args.argc - 1];
        FILE *fp = fopen(filepointer, "w");
        // Make sure that we have write permission to the file
        if (fp == NULL) {
          fprintf(stderr, "Cannot write to file %ss.\n", filepointer);
          fflush(stderr);
          _exit(0);
        } else {
          dup2(fileno(fp), STDOUT_FILENO);
          args.argv[args.argc - 1] = NULL;
        }
      }
      // Check if there is an alias that exists that matches the first argument
      char *alias = decode_alias(args.argv[0]);
      if (alias != NULL) {
        argument_c alias_args = parse_command(alias);
        for (int i = 1; i < args.argc; i++) {
          alias_args.argv[alias_args.argc] = args.argv[i];
          alias_args.argc++;
        }
        // Execute the command
        execv(alias_args.argv[0], alias_args.argv);
        // If this is reached then execv failed
        error_message(strcat(alias_args.argv[0], ": Command not found.\n"));
        _exit(0);
      } else {
        execv(args.argv[0], args.argv);
        // If this is reached then execv failed
        error_message(strcat(args.argv[0], ": Command not found.\n"));
        _exit(0);
      }
    } else {
      // Parent Process
      waitpid(pid, NULL, 0);
    }
  }
}

/*
* Batch mode driver for if the shell was run with a file 
*/
void batch_mode(char *cmd_line, FILE *fp) {
  argument_c args;
  while (fgets(cmd_line, MAX_CMD_LINE, fp) != NULL) {
    write(STDOUT_FILENO, cmd_line, strlen(cmd_line));
    if (strlen(cmd_line) == MAX_CMD_LINE - 1) {
      error_message("Command too long!\n");
    } else {
      args = parse_command(cmd_line);
      if (args.valid == 1) {
        run_command(args);
        free(args.argv);
      }
    }
  }
  // free_aliases();
}

/*
* Interactive mode driver for if the shell was run without a file
*/
void interactive_mode(char *cmd_line) {
  argument_c args;
  char *prompt = "mysh> ";
  while (write(STDOUT_FILENO, prompt, strlen(prompt)) &&
         fgets(cmd_line, MAX_CMD_LINE, stdin)) {
    if (strlen(cmd_line) == MAX_CMD_LINE - 1) {
      error_message("Command too long!\n");
    } else if (strcmp(cmd_line, "\n") != 0) {
      args = parse_command(cmd_line);
      if (args.valid == 1) {
        run_command(args);
        free(args.argv);
      }
    }
  }
  // free_aliases();
}

/*
* Main driver for the shell that calls batch_mode or interactive mode depending
* on how the user called the program
*/
int main(int argc, char *argv[]) {
  char cmd_line[MAX_CMD_LINE];
  if (argc < 1 || argc > 2) {
    error_message("Usage: mysh [batch-file]\n");
    exit(1);
  } else if (argc == 2) {
    // Batch Mode
    FILE *fp;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      fprintf(stderr, "Error: Cannot open file %s.\n", argv[1]);
      fflush(stderr);
      exit(1);
    }
    // Run batch mode
    batch_mode(cmd_line, fp);
    fclose(fp);
    exit(0);
  } else {
    // Run interactive mode
    interactive_mode(cmd_line);
    exit(0);
  }
}
