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
