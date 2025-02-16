#include "dshlib.h"
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

int process_cmd_buff(char *cmd_line, cmd_buff_t *buf) {
  if (strlen(cmd_line) == 0) {
    return WARN_NO_CMDS;
  }

  assert(buf != NULL);

  memset(buf, 0, sizeof(*buf));

  char *cmd_save;
  char *cmd = strtok_r(cmd_line, PIPE_STRING, &cmd_save);
  while (cmd != NULL) {
    // if (clist->num == CMD_MAX) {
    //   return ERR_TOO_MANY_COMMANDS;
    // }
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
      return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    // FIX: when multiple commands are implemented, this needs to change
    buf->argv[buf->argc++] = strdup(exe);
    // strcpy(buf->argv[buf->argc], exe);

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
            buf->argv[buf->argc++] = strdup(arg_start);
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
        buf->argv[buf->argc++] = strdup(arg_start);
      }
    }
    cmd = strtok_r(NULL, PIPE_STRING, &cmd_save);
  }
  return OK;
}

/*
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 *
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 */
int exec_local_cmd_loop() {
  char *cmd_buff;
  int rc = 0;
  cmd_buff_t cmd;

  cmd_buff = malloc(SH_CMD_MAX);

  bool terminate_shell = false;
  while (1) {
    printf("%s", SH_PROMPT);
    if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
      printf("\n");
      break;
    }
    cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

    rc = process_cmd_buff(cmd_buff, &cmd);

    switch (rc) {
    case OK: {
      char *exe = cmd.argv[0];
      if (strcmp(exe, EXIT_CMD) == 0) {
        terminate_shell = true;
      } else if (strcmp(exe, "dragon") == 0) {
        print_dragon();
      } else if (strcmp(exe, "cd") == 0) {
        if (cmd.argc == 2) {
          if (chdir(cmd.argv[1]) != 0) {
            // printf(CMD_ERR_EXECUTE);
          }
        }
      } else {
        pid_t child = fork();
        switch (child) {
        case -1: {
          // printf(CMD_ERR_EXECUTE);
          break;
        }
        case 0: {
          // child
          execvp(cmd.argv[0], cmd.argv);

          // Would only execute if there was an error
          //  printf(CMD_ERR_EXECUTE);
          printf("HI");
          exit(1);
        }
        default: {
          // parent
          int status;
          pid_t end;
          do {
            end = waitpid(child, &status, WUNTRACED);
            if (end == -1) {
              // Error from waitpid
              //  printf(CMD_ERR_EXECUTE);
              break;
            }
          } while (!WIFEXITED(status) && !WIFSIGNALED(status));
          if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            //  printf(CMD_ERR_EXECUTE);
          }
        }
        }
      }
      for (int i = 0; i < cmd.argc; i++) {
        free(cmd.argv[i]);
      }
      break;
    }
    case WARN_NO_CMDS: {
      printf(CMD_WARN_NO_CMD);
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
