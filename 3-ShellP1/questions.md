1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() prevents buffer overflows by only reading in a specific number of characters. It's a good choice for shells which read line by line since it stops reading when it encounters a newline.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  cmd_buff needs to use malloc since the input length can easily be longer than the input buffer. This would help prevent stack overflows and allow for what is essentially infinite length commands to be processed.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming the spaces from the command prevent parsing errors and avoids storing empty malformed tokens in the command struct.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  The < operator is used to redirect the input, reading from a file instead of the standard input. The > operator is used to redirect the output to a file. The 2> operator redirects the error to a file. For the input redirection, the shell needs to check for a valid file and reading errors. For the output redirections, the files would have to be checked/created and insure that the permissions are correct. Additionally for the errors need to be stored separately to insure that they are redirected to the correct file.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  While redirection changes the source/target of a command to a file, piping connects the output of one command to another's input. This allows for commands to be chained for more complex processing.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  By keeping the two separate, users are able to view errors and output independently. This allows for better debugging when issues arise since the output can be ignored.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**: For each command executed by the shell, the exit code can be kept track of. This allows the errors to be printed separately after all of the commands have been executed. The shell should offer the ability to merge STDOUT and STDERR through the use of 2>&1. 
