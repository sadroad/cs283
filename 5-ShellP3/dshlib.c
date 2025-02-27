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

void free_command_list(command_list_t *clist, int up_to_command) {
  for (int i = 0; i < up_to_command; i++) {
    for (int j = 0; j < clist->commands[i].argc; j++) {
      free(clist->commands[i].argv[j]);
    }
  }
}

void free_cmd_buffer(cmd_buff_t *buf, int up_to_arg) {
  for (int i = 0; i < up_to_arg; i++) {
    free(buf->argv[i]);
  }
}

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

int parse_command(char *cmd, cmd_buff_t *buf) {
  char *next_s = cmd;
  char *curr = next_s;
  char *exe;

  if (*curr == QUOTE_CHAR) {
    curr++; // Opening quote skipped
    exe = curr;
    while (*curr && *curr != QUOTE_CHAR) {
      curr++;
    }
    if (*curr == QUOTE_CHAR) {
      *curr = '\0';
      next_s = curr + 1;
    }
  } else {
    exe = strsep(&next_s, SPACE_STRING);
  }

  if (strlen(exe) >= EXE_MAX || (next_s != NULL && strlen(next_s) >= ARG_MAX)) {
    return ERR_CMD_OR_ARGS_TOO_BIG;
  }

  memset(buf, 0, sizeof(*buf));

  char *exe_copy = strdup(exe);
  if (exe_copy == NULL) {
    return ERR_MEMORY;
  }
  buf->argv[buf->argc++] = exe_copy;

  if (next_s != NULL) {
    bool in_quotes = false;
    char *arg_start = next_s;
    char *write_ptr = next_s;

    while (*next_s) {
      if (*next_s == QUOTE_CHAR) {
        in_quotes = !in_quotes;
        next_s++; // Skips quote character
        continue;
      }

      if (!in_quotes && *next_s == SPACE_CHAR) {
        *write_ptr = '\0';
        if (arg_start != write_ptr) {
          if (buf->argc >= ARG_MAX - 1) {
            // Clean up already allocated memory
            for (int i = 0; i < buf->argc; i++) {
              free(buf->argv[i]);
            }
            return ERR_CMD_OR_ARGS_TOO_BIG;
          }
          char *arg_copy = strdup(arg_start);
          if (arg_copy == NULL) {
            // Clean up already allocated memory
            for (int i = 0; i < buf->argc; i++) {
              free(buf->argv[i]);
            }
            return ERR_MEMORY;
          }
          buf->argv[buf->argc++] = arg_copy;
        }

        next_s++;
        while (*next_s && *next_s == SPACE_CHAR) {
          next_s++;
        }

        write_ptr++;
        arg_start = write_ptr;
        continue;
      }

      // Copy character, potentially overwriting quotes
      *write_ptr++ = *next_s++;
    }

    // Handle final argument if present
    *write_ptr = '\0';
    if (arg_start != write_ptr) {
      if (buf->argc >= ARG_MAX - 1) {
        for (int i = 0; i < buf->argc; i++) {
          free(buf->argv[i]);
        }
        return ERR_CMD_OR_ARGS_TOO_BIG;
      }
      char *arg_copy = strdup(arg_start);
      if (arg_copy == NULL) {
        for (int i = 0; i < buf->argc; i++) {
          free(buf->argv[i]);
        }
        return ERR_MEMORY;
      }
      buf->argv[buf->argc++] = arg_copy;
    }
  }

  buf->argv[buf->argc] = NULL;
  return OK;
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
      free_command_list(clist, clist->num);
      return ERR_TOO_MANY_COMMANDS;
    }
    remove_whitespace(cmd);

    cmd_buff_t buf = {0};
    int result = parse_command(cmd, &buf);

    if (result != OK) {
      // Clean up before returning error
      free_cmd_buffer(&buf, buf.argc);
      free_command_list(clist, clist->num);
      return result;
    }

    memcpy(&clist->commands[clist->num++], &buf, sizeof(buf));
    cmd = strtok_r(NULL, PIPE_STRING, &cmd_save);
  }

  return OK;
}

bool handle_builtin_command(char *exe, int argc, char **argv, int *last_rc,
                            bool *terminate_shell) {
  if (strcmp(exe, EXIT_CMD) == 0) {
    *terminate_shell = true;
    return true;
  } else if (strcmp(exe, "dragon") == 0) {
    print_dragon();
    return true;
  } else if (strcmp(exe, "cd") == 0) {
    if (argc == 2) {
      if (chdir(argv[1]) != 0) {
        fprintf(stderr, "Error in cd: %s\n", strerror(errno));
        *last_rc = errno;
      } else {
        *last_rc = OK;
      }
    }
    return true;
  } else if (strcmp(exe, "rc") == 0) {
    printf("%d\n", *last_rc);
    return true;
  }

  return false;
}

void handle_exec_error(int err) {
  switch (err) {
  case ENOENT:
    fprintf(stderr, "Command not found in PATH\n");
    break;
  case EACCES:
    fprintf(stderr, "Permission denied\n");
    break;
  default:
    fprintf(stderr, "Execution error: %s\n", strerror(err));
    break;
  }
  exit(err);
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

int execute_pipeline(command_list_t *clist, bool *terminate_shell) {
  int pipes[CMD_MAX - 1][2];
  pid_t pids[CMD_MAX];
  int last_rc = 0;

  if (clist->num > 0 && strcmp(clist->commands[0].argv[0], "cd") == 0) {
    if (clist->commands[0].argc == 2) {
      if (chdir(clist->commands[0].argv[1]) != 0) {
        fprintf(stderr, "Error in cd: %s\n", strerror(errno));
        return errno;
      }
    }

    if (clist->num == 1) {
      return 0;
    }

    for (int i = 0; i < clist->num - 1; i++) {
      clist->commands[i] = clist->commands[i + 1];
    }
    clist->num--;
  }

  for (int i = 0; i < clist->num; i++) {
    if (strcmp(clist->commands[i].argv[0], EXIT_CMD) == 0) {
      if (i == clist->num - 1) {
        *terminate_shell = true;
      }
    }
  }

  for (int i = 0; i < clist->num - 1; i++) {
    if (pipe(pipes[i]) == -1) {
      perror("pipe");
      return ERR_EXEC_CMD;
    }
  }

  for (int i = 0; i < clist->num; i++) {
    pids[i] = fork();

    if (pids[i] < 0) {
      perror("fork");

      for (int j = 0; j < clist->num - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      return ERR_EXEC_CMD;
    }

    if (pids[i] == 0) {
      // child

      if (i > 0) {
        if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
          perror("dup2 stdin");
          exit(1);
        }
      }

      if (i < clist->num - 1) {
        if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
          perror("dup2 stdout");
          exit(1);
        }
      }

      for (int j = 0; j < clist->num - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      char *exe = clist->commands[i].argv[0];

      if (strcmp(exe, "dragon") == 0) {
        print_dragon();
        exit(0);
      } else if (strcmp(exe, "cd") == 0) {
        // cd in pipeline is now handled by parent process
        exit(0);
      } else if (strcmp(exe, "rc") == 0) {
        printf("%d\n", last_rc);
        exit(0);
      } else if (strcmp(exe, EXIT_CMD) == 0) {
        exit(0);
      }

      execvp(exe, clist->commands[i].argv);

      int err = errno;
      switch (err) {
      case ENOENT:
        fprintf(stderr, "Command not found in PATH\n");
        break;
      case EACCES:
        fprintf(stderr, "Permission denied\n");
        break;
      default:
        fprintf(stderr, "Execution error: %s\n", strerror(err));
        break;
      }
      exit(err);
    }
  }

  for (int i = 0; i < clist->num - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  int status;
  for (int i = 0; i < clist->num; i++) {
    if (waitpid(pids[i], &status, 0) == -1) {
      perror("waitpid");
      return ERR_EXEC_CMD;
    }

    if (i == clist->num - 1) {
      if (WIFEXITED(status)) {
        last_rc = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        last_rc = 128 + WTERMSIG(status);
        fprintf(stderr, "Child terminated by signal %d\n", WTERMSIG(status));
      } else {
        last_rc = 1;
      }
    }
  }

  return last_rc;
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
      if (clist.num == 1) {
        int argc = clist.commands[0].argc;
        char **argv = clist.commands[0].argv;
        exec_command(argc, argv, &last_rc, &terminate_shell);
      } else {
        last_rc = execute_pipeline(&clist, &terminate_shell);
      }

      for (int i = 0; i < clist.num; i++) {
        for (int j = 0; j < clist.commands[i].argc; j++) {
          free(clist.commands[i].argv[j]);
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
    case ERR_CMD_OR_ARGS_TOO_BIG: {
      printf("error: command or arguments exceeded size limits\n");
      break;
    }
    case ERR_MEMORY: {
      printf("error: memory allocation failed\n");
      break;
    }
    default: {
      printf("error: unknown error processing command\n");
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
