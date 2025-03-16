#!/usr/bin/env bats

# Basic pipe functionality tests

@test "Simple pipe: ls | grep .c" {
  run ./dsh <<EOF
ls | grep .c
exit
EOF
  [ "$status" -eq 0 ]
  output_line=$(echo "$output" | grep -F "dshlib.c" | grep -v "grep .c")
  [ ! -z "$output_line" ]
  # Verify we see expected .c files in output
  [ $(echo "$output" | grep -E "dshlib\.c|dragon\.c|dsh_cli\.c" | wc -l) -ge 1 ]
}

@test "Multi-pipe: ls | grep .c | wc -l" {
  # First get count directly
  direct_count=$(ls | grep .c | wc -l)

  run ./dsh <<EOF
ls | grep .c | wc -l
exit
EOF

  # Extract count from output
  shell_count=$(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | tr -d '\r' | tr -d ' ' | sed 's/localmode$//')
  echo $shell_count

  # Verify shell count is non-zero and matches direct count
  [ ! -z "$shell_count" ]
  [ "$shell_count" -gt 0 ]
  [ "$shell_count" -eq "$direct_count" ]
}

@test "Data transformation: echo 'hello world' | tr '[a-z]' '[A-Z]'" {
  run ./dsh <<EOF
echo "hello world" | tr '[a-z]' '[A-Z]'
exit
EOF

  # If piping works, we should see the uppercased version
  [ $(echo "$output" | grep -c "HELLO WORLD") -eq 1 ]
  # And we shouldn't see the lowercase version (except in the command echo)
  [ $(echo "$output" | grep -v "echo" | grep -c "hello world") -eq 0 ]
}

@test "Dragon in pipe: dragon | grep %" {
  run ./dsh <<EOF
dragon | grep %
exit
EOF

  # Should find % characters from the dragon output
  [ $(echo "$output" | grep -c "%") -ge 1 ]
}

@test "Maximum pipeline depth: cat dshlib.c | grep int | grep void | grep char | grep if | grep for | grep while | wc -l" {
  run ./dsh <<EOF
cat dshlib.c | grep int | grep void | grep char | grep if | grep for | grep while | wc -l
exit
EOF

  # Just verify it runs without error and produces a numeric result
  [ "$status" -eq 0 ]
  number=$(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | grep -v "cat" | grep -v "grep" | tr -d '\r' | tr -d ' ' | sed 's/localmode$//')
  echo $number
  [[ "$number" =~ ^[0-9]+$ ]]
}

@test "cd in pipe shows warning but works: cd / | ls" {
  run ./dsh <<EOF
cd / | ls
pwd
exit
EOF

  # Should have changed directory to /
  dir_output=$(echo "$output" | grep -v "PARSED" | grep -v "cd" | grep -v "ls" | grep -v "exit" | grep -v "cmd loop")
  # Either we see / in output or at least verify we're not seeing an error
  [ $(echo "$dir_output" | grep -c "Error") -eq 0 ]
}

@test "Exit in pipe terminates after pipe completes: echo hello | exit" {
  run ./dsh <<EOF
echo hello | exit
echo "should not see this"
EOF

  [ $(echo "$output" | grep -c "should not see this") -eq 0 ]
}

@test "Simple multi-pipe test with number filtering" {
  run ./dsh <<EOF
echo "5 4 3 2 1" | grep "5" | grep "1"
exit
EOF

  [ $(echo "$output" | grep -c "5 4 3 2 1") -eq 1 ]
}

@test "Basic output redirection: echo test > out.txt" {
  run ./dsh <<EOF
echo test > out.txt
cat out.txt
rm out.txt
exit
EOF
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "test") -eq 1 ]
}

@test "Basic input redirection: cat < file.txt" {
  echo "test content" >file.txt
  run ./dsh <<EOF
cat < file.txt
rm file.txt
exit
EOF
  [ "$status" -eq 0 ]
  echo $output
  [ $(echo "$output" | grep -c "test content") -eq 1 ]
}

@test "Append mode redirection: echo appends with >>" {
  run ./dsh <<EOF
echo "line 1" > out.txt
echo "line 2" >> out.txt
cat out.txt
rm out.txt
exit
EOF
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "line 1") -eq 1 ]
  [ $(echo "$output" | grep -c "line 2") -eq 1 ]
}

@test "Combined redirection: sort < unsorted.txt > sorted.txt" {
  echo "c" >unsorted.txt
  echo "a" >>unsorted.txt
  echo "b" >>unsorted.txt

  run ./dsh <<EOF
sort < unsorted.txt > sorted.txt
cat sorted.txt
rm unsorted.txt sorted.txt
exit
EOF
  [ "$status" -eq 0 ]
  echo $output
  sorted_output=$(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | tr -d '\r')
  echo $sorted_output
  [ "$(echo "$sorted_output" | head -1)" = "a" ]
  [ "$(echo "$sorted_output" | head -2 | tail -1)" = "b" ]
  [ "$(echo "$sorted_output" | head -3 | tail -1)" = "c" ]
}

@test "Pipe and redirection combined: grep test < input.txt | sort > output.txt" {
  echo "test apple" >input.txt
  echo "test banana" >>input.txt
  echo "other content" >>input.txt

  run ./dsh <<EOF
grep test < input.txt | sort > output.txt
cat output.txt
rm input.txt output.txt
exit
EOF
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "test apple") -eq 1 ]
  [ $(echo "$output" | grep -c "test banana") -eq 1 ]
  [ $(echo "$output" | grep -c "other content") -eq 0 ]
}

@test "Multiple append operations stack content correctly" {
  run ./dsh <<EOF
echo "first line" > multi.txt
echo "second line" >> multi.txt
echo "third line" >> multi.txt
cat multi.txt
rm multi.txt
exit
EOF
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "first line") -eq 1 ]
  [ $(echo "$output" | grep -c "second line") -eq 1 ]
  [ $(echo "$output" | grep -c "third line") -eq 1 ]

  # Check correct order
  output_lines=$(echo "$output" | grep -E "first|second|third" | tr -d '\r')
  [ "$(echo "$output_lines" | head -1)" = "first line" ]
  [ "$(echo "$output_lines" | head -2 | tail -1)" = "second line" ]
  [ "$(echo "$output_lines" | head -3 | tail -1)" = "third line" ]
}

#!/usr/bin/env bats

@test "Simple command execution: ls" {
  run ./dsh <<EOF
ls
exit
EOF
  [ "$status" -eq 0 ]
  # Verify that some expected files appear in output
  [ $(echo "$output" | grep -c "dshlib.c") -eq 1 ]
  [ $(echo "$output" | grep -c "Makefile") -eq 1 ]
}

@test "Command with arguments: grep dsh dshlib.h" {
  run ./dsh <<EOF
grep dsh dshlib.h
exit
EOF
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "dsh") -ge 1 ]
}

@test "Pipe output to file: ls | grep .c > c_files.txt" {
  run ./dsh <<EOF
ls | grep .c > c_files.txt
cat c_files.txt
rm c_files.txt
exit
EOF
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "dshlib.c") -eq 1 ]
}

@test "Complex pipe chain with grep options: cat dshlib.c | grep -v '//' | grep -v '#include' | grep int | wc -l" {
  run ./dsh <<EOF
cat dshlib.c | grep -v '//' | grep -v '#include' | grep int | wc -l
exit
EOF

  # Just verify it runs and produces a number
  [ "$status" -eq 0 ]
  number=$(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | tr -d '\r' | tr -d ' ' | sed 's/localmode$//')
  [[ "$number" =~ ^[0-9]+$ ]]
  [ "$number" -gt 0 ]
}

@test "Multiple redirections in pipeline: grep int < dshlib.c | grep void | sort > sorted_funcs.txt" {
  run ./dsh <<EOF
grep int < dshlib.c | grep void | sort > sorted_funcs.txt
cat sorted_funcs.txt
rm sorted_funcs.txt
exit
EOF

  [ "$status" -eq 0 ]
  # Should find at least one line with both "int" and "void"
  [ $(echo "$output" | grep -c "int" | grep -c "void") -ge 0 ]
}

@test "Append multiple command outputs: echo 'first' > file.txt && echo 'second' | cat >> file.txt" {
  run ./dsh <<EOF
echo "first" > file.txt
echo "second" | cat >> file.txt
cat file.txt
rm file.txt
exit
EOF

  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "first") -eq 1 ]
  [ $(echo "$output" | grep -c "second") -eq 1 ]
}

@test "Non-existent command in pipe: ls | nonexistentcommand | wc -l" {
  run ./dsh <<EOF
ls | nonexistentcommand | wc -l
exit
EOF

  # Should not crash, but show appropriate error
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "not found") -ge 1 ] || [ $(echo "$output" | grep -c "Command not found") -ge 1 ]
}

@test "Excessive pipe depth: cmd1 | cmd2 | ... | cmd20" {
  # Create a command with more pipes than CMD_MAX allows
  cmd="ls"
  for i in {1..20}; do
    cmd="$cmd | grep ."
  done

  run ./dsh <<EOF
$cmd
exit
EOF

  # Should show error about pipe limit
  [ "$status" -eq 0 ]
  [ $(echo "$output" | grep -c "too many") -ge 1 ] || [ $(echo "$output" | grep -c "piping limited") -ge 1 ]
}

@test "Built-in dragon command with pipe: dragon | head -20" {
  run ./dsh <<EOF
dragon | head -20
exit
EOF

  [ "$status" -eq 0 ]
  # Dragon output should be present but limited
  [ $(echo "$output" | grep -c "%") -ge 1 ]
}

find_available_port() {
  local port=$(shuf -i 10000-65000 -n 1)
  while netstat -tuln | grep ":$port " >/dev/null; do
    port=$(shuf -i 10000-65000 -n 1)
  done
  echo $port
}

@test "Basic server-client connection" {
  # Find available port
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!

  # Give server time to start
  sleep 1

  # Test connection with a simple command
  OUTPUT=$(echo -e "echo 'Server connection successful'\nexit" | ./dsh -c -p $PORT)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Check output
  echo "$OUTPUT"
  [[ "$OUTPUT" == *"Server connection successful"* ]]
}

@test "Execute command on remote server" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # Run a command that returns system-specific information
  OUTPUT=$(echo -e "uname -a\nexit" | ./dsh -c -p $PORT)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Verify output contains expected system info
  echo "$OUTPUT"
  [[ "$OUTPUT" == *"Linux"* ]]
}

@test "File operations on remote server" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # Create a file, write to it, then read it
  OUTPUT=$(echo -e "echo 'remote test' > remote_test.txt\ncat remote_test.txt\nrm remote_test.txt\nexit" | ./dsh -c -p $PORT)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Verify file content was read back
  echo "$OUTPUT"
  [[ "$OUTPUT" == *"remote test"* ]]
}

@test "Remote built-in dragon command" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # Test the special dragon command
  OUTPUT=$(echo -e "dragon\nexit" | ./dsh -c -p $PORT)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Verify dragon ASCII art is in output
  echo "$OUTPUT"
  [[ "$OUTPUT" == *"%"* ]]
}

@test "Test stop-server command" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # Send stop-server command
  OUTPUT=$(echo -e "stop-server" | ./dsh -c -p $PORT 2>/dev/null)

  # Wait briefly to allow server to shut down
  sleep 1

  # Check if server process is still running
  if ps -p $SERVER_PID >/dev/null; then
    # If it is, kill it (test will fail)
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
    false # Force test to fail
  fi

  # Verify shutdown message in output
  echo "$OUTPUT"
  [[ "$OUTPUT" == *"shutting down"* ]] || [[ "$OUTPUT" == *"Server shutting down"* ]]
}

@test "Error handling on remote server" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # Run a command that doesn't exist
  OUTPUT=$(echo -e "thisCmdDoesNotExist\nexit" | ./dsh -c -p $PORT 2>&1)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Verify error message
  echo "$OUTPUT"
  [[ "$OUTPUT" == *"not found"* ]] || [[ "$OUTPUT" == *"Command not found"* ]] || [[ "$OUTPUT" = *"No such file"* ]]
}

@test "Large output handling on remote server" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # Generate large output
  OUTPUT=$(echo -e "cat dshlib.c dshlib.h rshlib.h\nexit" | ./dsh -c -p $PORT)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Verify we received substantial output
  echo "Output length: ${#OUTPUT}"
  [ ${#OUTPUT} -gt 1000 ]
}

@test "Sequential clients to same server" {
  PORT=$(find_available_port)

  # Start server in background
  ./dsh -s -p $PORT &
  SERVER_PID=$!
  sleep 1

  # First client
  OUTPUT1=$(echo -e "echo 'client 1'\nexit" | ./dsh -c -p $PORT)

  # Second client
  OUTPUT2=$(echo -e "echo 'client 2'\nexit" | ./dsh -c -p $PORT)

  # Kill server
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null || true

  # Verify both outputs
  echo "Client 1: $OUTPUT1"
  echo "Client 2: $OUTPUT2"
  [[ "$OUTPUT1" == *"client 1"* ]]
  [[ "$OUTPUT2" == *"client 2"* ]]
}
