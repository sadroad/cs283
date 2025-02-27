1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

My implementation waits by using a loop that calls waitpid() for each child PID. This blocks the parent until all of the children have finished. If waitpid() wasn't called on every child process then zombie processes could form.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

If unused pipes are not closed after using dup2(), open file descriptors could lead to resource leaks and unexpected behavior. For example, if a pipe is left open, the process might hang because it is waiting for input from the pipe that will never come. Additionally, leaving pipes open can exhaust the system's file descriptor limit, causing subsequent operations to fail.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

cd has to be implemented as a built-in command because it needs to change the directory of the shell itself. If cd was implemented as an external process, it would change the child process directory and then terminate leaving the parent shell in the original directory.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

I would replace the fixed-size arrays with arrays created through malloc(). They would be freed when the pipeline execution is complete. This would require more complex memory management and slightly worse performance due to the overhead of calling malloc().
