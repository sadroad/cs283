#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

// INCLUDES for extra credit
// #include <signal.h>
// #include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"

#ifdef DEBUG
#define DEBUG_SOCKET_REUSE                                                     \
  int enable = 1;                                                              \
  setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
#else
#define DEBUG_SOCKET_REUSE
#endif

/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 *
 *      port:   The port the server will use.  Note the constant
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port
 *
 *      is_threded:  Used for extra credit to indicate the server should
 * implement per thread connections for clients
 *
 *      This function basically runs the server by:
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server.
 *
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.
 *
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.
 */
int start_server(char *ifaces, int port, int is_threaded) {
  int svr_socket;
  int rc;

  //
  // TODO:  If you are implementing the extra credit, please add logic
  //       to keep track of is_threaded to handle this feature
  //

  svr_socket = boot_server(ifaces, port);
  if (svr_socket < 0) {
    int err_code = svr_socket; // server socket will carry error code
    return err_code;
  }

  rc = process_cli_requests(svr_socket);

  stop_server(svr_socket);

  return rc;
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function.
 *
 *      This function simply returns the value of close() when closing
 *      the socket.
 */
int stop_server(int svr_socket) { return close(svr_socket); }

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.
 *
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically:
 *
 *      1. Create the server socket using the socket() function. 2. Calling bind
 * to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 *
 *      after creating the socket and prior to calling bind you might want to
 *      include the following code:
 *
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 *
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 *
 *  Returns:
 *
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor,
 *                      which is just an integer.
 *
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails.
 *
 */
int boot_server(char *ifaces, int port) {
  int svr_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (svr_socket == -1) {
    return ERR_RDSH_COMMUNICATION;
  }

  DEBUG_SOCKET_REUSE;

  struct sockaddr_in addr;

  addr.sin_family = AF_INET;

  int result = inet_pton(AF_INET, ifaces, &(addr.sin_addr));

  if (result != 1) {
    perror("Bad ip format");
    return ERR_RDSH_COMMUNICATION;
  }

  addr.sin_addr.s_addr = htonl(addr.sin_addr.s_addr);
  addr.sin_port = htons(port);

  result = bind(svr_socket, (const struct sockaddr *)&addr,
                sizeof(struct sockaddr_in));
  if (result == -1) {
    return ERR_RDSH_COMMUNICATION;
  }

  result = listen(svr_socket, 20);
  if (result == -1) {
    return ERR_RDSH_COMMUNICATION;
  }

  return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *
 *  This function handles managing client connections.  It does this using
 *  the following logic
 *
 *      1.  Starts a while(1) loop:
 *
 *          a. Calls accept() to wait for a client connection. Recall that
 *             the accept() function returns another socket specifically
 *             bound to a client connection.
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.
 *
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop.
 *
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket.
 *
 *  Returns:
 *
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT.
 *
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept()
 *                function failing.
 *
 *      OTHERS:   See exec_client_requests() for return codes.  Note that
 * positive values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server.
 *
 */
int process_cli_requests(int svr_socket) {
  int result;
  while (true) {
    int cli_socket = accept(svr_socket, NULL, NULL);
    if (cli_socket < 0) {
      return ERR_RDSH_COMMUNICATION;
    }
    result = exec_client_requests(cli_socket);
    close(cli_socket);
    if (result < 0) {
      break;
    }
  }
  return result;
}

/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 *
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection.
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop.
 *
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard.
 *
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish
 *  this.
 *
 *  Of final note, this function must allocate a buffer for storage to
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 *
 *  Returns:
 *
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client.
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 *
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors.
 */
int exec_client_requests(int cli_socket) {
  char *buff = malloc(RDSH_COMM_BUFF_SZ);
  if (!buff) {
    return ERR_RDSH_COMMUNICATION;
  }

  command_list_t clist;
  bool terminate_shell = false;
  int rc = 0;

  while (true) {
    int bytes_received = recv(cli_socket, buff, RDSH_COMM_BUFF_SZ, 0);
    if (bytes_received <= 0) {
      free(buff);
      return ERR_RDSH_COMMUNICATION;
    }

    if (bytes_received < RDSH_COMM_BUFF_SZ) {
      buff[bytes_received] = '\0';
    }

    size_t len = strlen(buff);
    if (len > 0 && buff[len - 1] == '\n') {
      buff[len - 1] = '\0';
    }

    buff[bytes_received - 1] = '\0';

    if (strcmp(buff, "stop-server") == 0) {
      send_message_string(cli_socket, "Server shutting down...\n");
      free(buff);
      return OK_EXIT;
    } else if (strcmp(buff, "exit") == 0) {
      send_message_string(cli_socket, "Connection closed\n");
      free(buff);
      return OK;
    } else if (strcmp(buff, "dragon") == 0) {
      int stdout_backup = dup(STDOUT_FILENO);
      int pipe_fds[2];
      pipe(pipe_fds);
      dup2(pipe_fds[1], STDOUT_FILENO);
      close(pipe_fds[1]);

      print_dragon();
      fflush(stdout);

      dup2(stdout_backup, STDOUT_FILENO);
      close(stdout_backup);

      char dragon_buff[RDSH_COMM_BUFF_SZ];
      int bytes_read = read(pipe_fds[0], dragon_buff, RDSH_COMM_BUFF_SZ - 1);
      close(pipe_fds[0]);

      if (bytes_read > 0) {
        dragon_buff[bytes_read] = '\0';
        send(cli_socket, dragon_buff, bytes_read, 0);
      }

      if (send_message_eof(cli_socket) != OK) {
        free(buff);
        return ERR_RDSH_COMMUNICATION;
      }
      continue;
    }

    rc = process_cmd_buff(buff, &clist);

    if (rc == WARN_NO_CMDS) {
      send_message_string(cli_socket, CMD_WARN_NO_CMD);
      continue;
    } else if (rc == ERR_TOO_MANY_COMMANDS) {
      char error_msg[100];
      snprintf(error_msg, sizeof(error_msg), CMD_ERR_PIPE_LIMIT, CMD_MAX);
      send_message_string(cli_socket, error_msg);
      continue;
    } else if (rc != OK) {
      send_message_string(cli_socket, "Command parsing error\n");
      continue;
    }

    // MODIFIED: Execute pipeline if multiple commands, otherwise use standard
    // handling
    if (clist.num > 1) {
      // Use our new pipeline implementation
      int stdout_backup = dup(STDOUT_FILENO);
      int stderr_backup = dup(STDERR_FILENO);

      // Create pipe to capture output
      int pipe_fds[2];
      if (pipe(pipe_fds) == -1) {
        send_message_string(cli_socket, "Failed to create output pipe\n");
        continue;
      }

      // Redirect stdout and stderr to pipe
      dup2(pipe_fds[1], STDOUT_FILENO);
      dup2(pipe_fds[1], STDERR_FILENO);
      close(pipe_fds[1]);

      // Execute the pipeline
      rsh_execute_pipeline(pipe_fds[1], &clist);

      // Restore stdout and stderr
      dup2(stdout_backup, STDOUT_FILENO);
      dup2(stderr_backup, STDERR_FILENO);
      close(stdout_backup);
      close(stderr_backup);

      // Send captured output to client
      char output_buff[RDSH_COMM_BUFF_SZ];
      ssize_t bytes_read;
      while ((bytes_read = read(pipe_fds[0], output_buff,
                                sizeof(output_buff) - 1)) > 0) {
        output_buff[bytes_read] = '\0';
        if (send(cli_socket, output_buff, bytes_read, 0) < 0) {
          break;
        }
      }
      close(pipe_fds[0]);

      // Send EOF to client
      if (send_message_eof(cli_socket) != OK) {
        free(buff);
        return ERR_RDSH_COMMUNICATION;
      }
    } else {
      // Single command - use existing handler
      // [keep existing code for single command execution]
      // Fork and execute the command, capturing output
      int pipefd[2];
      if (pipe(pipefd) == -1) {
        send_message_string(cli_socket, "Failed to create pipe\n");
        continue;
      }

      pid_t pid = fork();
      if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        send_message_string(cli_socket, "Failed to fork process\n");
        continue;
      }

      if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close read end

        // Redirect stdout and stderr to the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        // Handle built-in commands like cd
        if (strcmp(clist.commands[0].argv[0], "cd") == 0) {
          if (clist.commands[0].argc > 1) {
            if (chdir(clist.commands[0].argv[1]) != 0) {
              fprintf(stderr, "cd: %s: %s\n", clist.commands[0].argv[1],
                      strerror(errno));
              exit(1);
            }
          }
          exit(0);
        } else {
          execvp(clist.commands[0].argv[0], clist.commands[0].argv);
          fprintf(stderr, "Command execution failed: %s\n", strerror(errno));
          exit(1);
        }
      }

      // Parent process
      close(pipefd[1]); // Close write end

      // Read from pipe and send to client
      char output_buff[RDSH_COMM_BUFF_SZ];
      ssize_t n;
      while ((n = read(pipefd[0], output_buff, sizeof(output_buff) - 1)) > 0) {
        output_buff[n] = '\0';
        if (send(cli_socket, output_buff, n, 0) < 0) {
          break;
        }
      }
      close(pipefd[0]);

      // Wait for the child to finish
      int status;
      waitpid(pid, &status, 0);

      // Send EOF to client
      if (send_message_eof(cli_socket) != OK) {
        free(buff);
        return ERR_RDSH_COMMUNICATION;
      }
    }

    // Clean up command list
    for (int i = 0; i < clist.num; i++) {
      for (int j = 0; j < clist.commands[i].argc; j++) {
        free(clist.commands[i].argv[j]);
      }
      free(clist.commands[i].input_file);
      free(clist.commands[i].output_file);
    }
  }

  free(buff);
  return OK;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent.
 *
 *  Returns:
 *
 *      OK:  The EOF character was sent successfully.
 *
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character.
 */
int send_message_eof(int cli_socket) {
  int result = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
  if (result == -1) {
    return ERR_RDSH_COMMUNICATION;
  }
  return OK;
}

/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client.
 *
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated.
 *
 *  Returns:
 *
 *      OK:  The message in buff followed by the EOF character was
 *           sent successfully.
 *
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character.
 */
int send_message_string(int cli_socket, char *buff) {
  int result = send(cli_socket, buff, strlen(buff), 0);
  if (result == -1) {
    return ERR_RDSH_COMMUNICATION;
  }
  return send_message_eof(cli_socket);
}

/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell.
 *
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable.
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:
 *
 *
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │
 *      │   │              │     │              │     │              │  │  │
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │
 *          │              │ │   │              │ │   │              │     │
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘
 *          └──────────────┘     └──────────────┘     └──────────────┘
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function
 *
 *  Returns:
 *
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value.
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
  int pipes[CMD_MAX - 1][2];
  pid_t pids[CMD_MAX];
  int status, last_rc = 0;

  // Create necessary pipes
  for (int i = 0; i < clist->num - 1; i++) {
    if (pipe(pipes[i]) == -1) {
      perror("pipe");
      return ERR_RDSH_CMD_EXEC;
    }
  }

  // Fork and execute each command in the pipeline
  for (int i = 0; i < clist->num; i++) {
    pids[i] = fork();

    if (pids[i] < 0) {
      perror("fork");
      // Close all pipes
      for (int j = 0; j < clist->num - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      return ERR_RDSH_CMD_EXEC;
    }

    if (pids[i] == 0) {
      // Child process

      // Setup stdin from previous pipe (except for first command)
      if (i > 0) {
        if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
          perror("dup2 stdin");
          exit(1);
        }
      }

      // Setup stdout to next pipe (except for last command)
      if (i < clist->num - 1) {
        if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
          perror("dup2 stdout");
          exit(1);
        }
      } else {
        // Last command in pipeline - redirect stdout to client socket
        if (dup2(cli_sock, STDOUT_FILENO) == -1) {
          perror("dup2 stdout to socket");
          exit(1);
        }
        if (dup2(cli_sock, STDERR_FILENO) == -1) {
          perror("dup2 stderr to socket");
          exit(1);
        }
      }

      // Close all pipe fds in child
      for (int j = 0; j < clist->num - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      // Handle input/output redirection for the command
      if (clist->commands[i].input_file != NULL) {
        int fd = open(clist->commands[i].input_file, O_RDONLY);
        if (fd < 0) {
          perror("open input file");
          exit(errno);
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
          perror("dup2 input redirection");
          exit(errno);
        }
        close(fd);
      }

      if (clist->commands[i].output_file != NULL) {
        int flags = O_WRONLY | O_CREAT;
        if (clist->commands[i].append_mode) {
          flags |= O_APPEND;
        } else {
          flags |= O_TRUNC;
        }
        int fd = open(clist->commands[i].output_file, flags, 0644);
        if (fd < 0) {
          perror("open output file");
          exit(errno);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
          perror("dup2 output redirection");
          exit(errno);
        }
        close(fd);
      }

      // Execute the command
      char *exe = clist->commands[i].argv[0];

      // Handle built-in commands
      if (strcmp(exe, "dragon") == 0) {
        print_dragon();
        exit(0);
      } else if (strcmp(exe, "cd") == 0) {
        // cd in pipeline is handled by parent process
        exit(0);
      } else if (strcmp(exe, "rc") == 0) {
        printf("%d\n", last_rc);
        exit(0);
      } else if (strcmp(exe, EXIT_CMD) == 0) {
        exit(0);
      }

      execvp(exe, clist->commands[i].argv);

      // If we get here, exec failed
      fprintf(stderr, "Command execution failed: %s\n", strerror(errno));
      exit(errno);
    }
  }

  // Parent process - close all pipes
  for (int i = 0; i < clist->num - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  // Wait for all children to finish
  for (int i = 0; i < clist->num; i++) {
    if (waitpid(pids[i], &status, 0) == -1) {
      perror("waitpid");
      return ERR_RDSH_CMD_EXEC;
    }

    // Get exit status of last command in pipeline
    if (i == clist->num - 1) {
      if (WIFEXITED(status)) {
        last_rc = WEXITSTATUS(status);
      } else {
        last_rc = 1; // Error
      }
    }
  }

  return last_rc;
}

/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 ****
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 *
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 *
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation.
 *
 *  Returns:
 *
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 *
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned.
 */
Built_In_Cmds rsh_match_command(const char *input) {
  return BI_NOT_IMPLEMENTED;
}

/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the
 *            parsed version fo the command
 *
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.
 *
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.
 *
 *  Returns:
 *
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns
 * BI_EXECUTED BI_CMD_*     Indicates that a built-in command was matched and
 * the caller is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is
 * returned the caller is responsible for closing the client connection.
 *
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY.
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) { return BI_NOT_IMPLEMENTED; }
