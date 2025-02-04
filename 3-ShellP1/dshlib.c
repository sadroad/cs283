#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */

void remove_whitespace(char *s) {
  char *start = s;
  char *end;

  while (*start && *start == SPACE_CHAR) {
    start++;
  }

  if (*start == '\0') {
    *s = '\0';
    return;
  }

  end = start + strlen(start) - 1;

  while (end > start && *end == SPACE_CHAR) {
    end--;
  }

  *(end + 1) = '\0';

  if (start != s) {
    memmove(s, start, end - start + 2);
  }
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
  if (strlen(cmd_line) == 0) {
    return WARN_NO_CMDS;
  }

  if (clist == NULL) {
    exit(-1);
  }

  memset(clist, 0, sizeof(*clist));

  char *cmd_save;
  char *cmd = strtok_r(cmd_line, PIPE_STRING, &cmd_save);
  while (cmd != NULL) {
    if (clist->num == CMD_MAX) {
      return ERR_TOO_MANY_COMMANDS;
    }
    remove_whitespace(cmd);

    char *next_s = cmd;
    char *exe = strsep(&next_s, SPACE_STRING);

    if (strlen(exe) >= EXE_MAX ||
        (next_s != NULL && strlen(next_s) >= ARG_MAX)) {
      return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    strcpy(clist->commands[clist->num].exe, exe);
    if (next_s != NULL) {
      strcpy(clist->commands[clist->num].args, next_s);
    } else {
      clist->commands[clist->num].args[0] = '\0';
    }

    clist->num++;
    cmd = strtok_r(NULL, PIPE_STRING, &cmd_save);
  }
  return OK;
}
