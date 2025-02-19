1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  We have to use fork before calling execvp to preserve the shell's environment. execvp will replace the current process and if called without fork would lead to the loop/state being lost. To solve this fork is used and allows a child process to be overwritten. This lets the parent wait for the child to finish execution before continuing.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  When fork fails it will return a -1 and set the error to errno. I check the return value of fork and if it returns -1 then I update any return codes with the data from errno.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  execvp will search through all the directories listed in the PATH environment variable for the binary to execute.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  We call wait() to suspend the parent's execution until the child finishes execution. This prevents states where the child finishes running, but the parent hasn't checked the state of the child. By waiting for the child, zombie process can be avoided.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS returns the exit status of the child assuming the child terminated normally. This allows us perform actions depending on the status code, like setting the return code used in the rc command.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  My implementation of build_cmd_buff iterates over each character of the input and detects when a quote character is found. If a quote is found, a flag is toggled to ignore whitespace until another quite is found. When another quote is found, the internal string is copied as an argument. It avoids copying the quotes by overwriting itself essentially erasing the quotes. This is needed to allow for commands like echo to output content with spaces and not treat them as multiple arguments.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  Much of my parsing logic stayed the same when comparing to my prior implementation. The main change was handling quotes in both the arguments and the executable. It took me a little bit to figure out how to ignore the quotes without allocating more memory.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.or&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  Signals are a lightweight notifications that can be used to notify other processes about events. Compared to other forms of IPC then use less resources and carry less data.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGKILL forces a program to terminate and cannot be caught. It is used when SIGTERM is being ignored. SIGTERM requests a program to terminate gracefully. The program is allowed to catch/ignore the signal. This allows the program to perform cleanup actions. SIGINT requests a program to be interrupted. A common use case is pressing Ctrl + C in a shell to stop a running program. It can be caught to allow for custom confirmations before quitting.

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  SIGSTOP cannot be caught as it meant to reliably suspend a program for later use.
