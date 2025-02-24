#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dshlib.h"

void remove_whitespace(char *s) {
  char *start = s;
  char *end;

  while (*start && *start == SPACE_CHAR) {
    start++;
  }

  // If no content in the string
  if (*start == '\0') {
    *s = '\0';
    return;
  }

  end = start + strlen(start) - 1;

  while (end > start && *end == SPACE_CHAR) {
    end--;
  }

  *(end + 1) = '\0';

  // Shifts trimmed string to start of buffer
  if (start != s) {
    memmove(s, start, end - start + 2);
  }
}

int process_cmd_buff(char *cmd_line, command_list_t *clist) {
  if (strlen(cmd_line) == 0) {
    return WARN_NO_CMDS;
  }

  assert(clist != NULL);

  memset(clist, 0, sizeof(*clist));

  char *cmd_save;
  char *cmd = strtok_r(cmd_line, PIPE_STRING, &cmd_save);
  while (cmd != NULL) {
    if (clist->num == CMD_MAX) {
      for (int i = 0; i < clist->num; i++) {
        for (int j = 0; j < clist->commands[i].argc; j++) {
          free(clist->commands[i].argv[j]);
        }
      }
      return ERR_TOO_MANY_COMMANDS;
    }
    remove_whitespace(cmd);

    char *next_s = cmd;
    char *curr = next_s;
    char *exe;

    if (*curr == QUOTE_CHAR) {
      curr++; // Opening quote skipped
      exe = curr;
      while (*curr &&
             *curr != QUOTE_CHAR) { // Find when the quoted executable ends
        curr++;
      }
      if (*curr == QUOTE_CHAR) {
        *curr = '\0';
        next_s = curr + 1;
      }
      // TODO: figure out what to do when the executable doesn't have a matching
      // closing quote. Probably error out with invalid syntax
    } else {
      exe = strsep(&next_s, SPACE_STRING);
    }

    if (strlen(exe) >= EXE_MAX ||
        (next_s != NULL && strlen(next_s) >= ARG_MAX)) {
      for (int i = 0; i < clist->num; i++) {
        for (int j = 0; j < clist->commands[i].argc; j++) {
          free(clist->commands[i].argv[j]);
        }
      }
      return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    cmd_buff_t buf = {0};
    char *exe_copy = strdup(exe);
    if (exe_copy == NULL) {
      return ERR_MEMORY;
    }
    buf.argv[buf.argc++] = exe_copy;

    if (next_s != NULL) {
      bool in_quotes = false;
      char *arg_start = next_s;
      char *write_ptr = next_s;
      // This will use the previous parts of next_s to copy the valid arg to
      // Allows quotes to be ignored by writing overthem

      while (*next_s) {
        if (*next_s == QUOTE_CHAR) {
          in_quotes = !in_quotes;
          next_s++; // Skips quote character
          continue;
        }

        if (!in_quotes && *next_s == SPACE_CHAR) {
          *write_ptr = '\0';
          if (arg_start != write_ptr) {
            if (buf.argc >= ARG_MAX - 1) {
              for (int i = 0; i < buf.argc; i++) {
                free(buf.argv[i]);
              }
              return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            char *arg_copy = strdup(arg_start);
            if (arg_copy == NULL) {
              for (int i = 0; i < buf.argc; i++) {
                free(buf.argv[i]);
              }
              return ERR_MEMORY;
            }
            buf.argv[buf.argc++] = arg_copy;
          }

          next_s++;
          while (*next_s && *next_s == SPACE_CHAR) {
            next_s++;
          }

          write_ptr++;
          arg_start = write_ptr;
          continue;
        }

        // When this copys characters, if there was a quote previously, then the
        // quote will be written over with the content inside
        // w"hi" -> whhi" -> whii" -> copied to arg
        *write_ptr++ = *next_s++;
      }

      *write_ptr = '\0';
      if (arg_start != write_ptr) {

        if (buf.argc >= ARG_MAX - 1) {
          for (int i = 0; i < buf.argc; i++) {
            free(buf.argv[i]);
          }
          return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        char *arg_copy = strdup(arg_start);
        if (arg_copy == NULL) {
          for (int i = 0; i < buf.argc; i++) {
            free(buf.argv[i]);
          }
          return ERR_MEMORY;
        }
        buf.argv[buf.argc++] = arg_copy;
      }
    }
    buf.argv[buf.argc] = NULL;
    memcpy(&clist->commands[clist->num++], &buf, sizeof(buf));
    cmd = strtok_r(NULL, PIPE_STRING, &cmd_save);
  }
  return OK;
}

void exec_command(int argc, char **argv, int *last_rc, bool *terminate_shell) {
  char *exe = argv[0];
  if (strcmp(exe, EXIT_CMD) == 0) {
    *terminate_shell = true;
  } else if (strcmp(exe, "dragon") == 0) {
    print_dragon();
  } else if (strcmp(exe, "cd") == 0) {
    if (argc == 2) {
      if (chdir(argv[1]) != 0) {
        fprintf(stderr, "Error in cd: %s\n", strerror(errno));
        *last_rc = errno;
        return;
      }
    }
  } else if (strcmp(exe, "rc") == 0) {
    printf("%d\n", *last_rc);
  } else {
    pid_t child = fork();
    switch (child) {
    case -1: {
      // printf(CMD_ERR_EXECUTE);
      *last_rc = errno;
      return;
    }
    case 0: {
      // child
      execvp(exe, argv);
      int err = errno;
      switch (err) {
      case ENOENT: {
        fprintf(stderr, "Command not found in PATH\n");
        break;
      }
      case EACCES: {
        fprintf(stderr, "Permission denied\n");
        break;
      }
      default: {
        fprintf(stderr, "Exectuion error: %s\n", strerror(err));
        break;
      }
      }
      // Would only execute if there was an error
      //  printf(CMD_ERR_EXECUTE);
      exit(err);
    }
    default: {
      // parent
      int status;
      pid_t end = waitpid(child, &status, 0);
      if (end == -1) {
        perror("waitpid");
        *last_rc = errno;
        return;
      } else {
        if (WIFEXITED(status)) {
          *last_rc = WEXITSTATUS(status);
          return;
        } else if (WIFSIGNALED(status)) {
          *last_rc = WTERMSIG(status);
          fprintf(stderr, "Child terminated by signal %d\n", *last_rc);
          return;
        }
      }
    }
    }
  }
  *last_rc = OK;
}

int exec_local_cmd_loop() {
  char *cmd_buff;
  int rc = 0;
  command_list_t clist;

  int last_rc = 0;

  cmd_buff = malloc(SH_CMD_MAX);
  if (cmd_buff == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return ERR_MEMORY;
  }

  bool terminate_shell = false;
  while (1) {
    printf("%s", SH_PROMPT);
    if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
      printf("\n");
      break;
    }
    cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

    rc = process_cmd_buff(cmd_buff, &clist);

    switch (rc) {
    case OK: {
      printf(CMD_OK_HEADER, clist.num);
      for (int i = 0; i < clist.num; i++) {
        int argc = clist.commands[i].argc;
        char **argv = clist.commands[i].argv;
        exec_command(argc, argv, &last_rc, &terminate_shell);

        for (int j = 0; j < argc; j++) {
          free(argv[j]);
        }
      }
      break;
    }
    case WARN_NO_CMDS: {
      printf(CMD_WARN_NO_CMD);
      break;
    }
    case ERR_TOO_MANY_COMMANDS: {
      printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
      break;
    }
    }
    if (terminate_shell) {
      break;
    }
  }

  free(cmd_buff);
  return OK;
}
