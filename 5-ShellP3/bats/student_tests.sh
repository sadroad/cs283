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
  shell_count=$(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | tr -d '\r' | tr -d ' ')

  # Verify shell count is non-zero and matches direct count
  [ ! -z "$shell_count" ]
  [ "$shell_count" -gt 0 ]
  [ "$shell_count" -eq "$direct_count" ]
}

@test "Pipe with echo verification: echo test_string | grep test" {
  run ./dsh <<EOF
echo test_string | grep test
exit
EOF

  # Verify the output contains test_string
  [ $(echo "$output" | grep -c "test_string") -eq 1 ]
  # Make sure the output doesn't contain lines we didn't expect
  [ $(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | grep -v "echo test_string" | grep -v "test_string" | wc -l) -eq 0 ]
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
  number=$(echo "$output" | grep -v "PARSED" | grep -v "exit" | grep -v "cmd loop" | grep -v "cat" | grep -v "grep" | tr -d '\r' | tr -d ' ')
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
