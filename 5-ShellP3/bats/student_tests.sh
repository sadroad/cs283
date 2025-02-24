#!/usr/bin/env bats

# File: student_tests.sh
#
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
  run ./dsh <<EOF
ls
EOF

  # Assertions
  [ "$status" -eq 0 ]
}

# Old tests

@test "Exit command terminates shell" {
  run ./dsh <<EOF
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "cmd loop returned 0"
}

@test "Dragon command outputs ASCII art" {
  run ./dsh <<EOF
dragon
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "%"
}

@test "Blank input gives a warning" {
  run ./dsh <<EOF

exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "warning: no commands provided"
}

@test "Invalid external command shows error and sets rc" {
  run ./dsh <<EOF
someunknowncmd
rc
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "Command not found in PATH"
  echo "$output" | grep -E "2$"
}

@test "cd invalid directory shows error" {
  run ./dsh <<EOF
cd /nonexistent_directory
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "Error in cd:"
}

@test "cd valid directory changes working directory" {
  run ./dsh <<EOF
cd /
pwd
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "^/$"
}

@test "External command uname works" {
  run ./dsh <<EOF
uname -s
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -Ev "Command not found in PATH" >/dev/null
  echo "$output" | grep -qE "Linux|Darwin"
}

@test "rc built-in returns 0 after successful command" {
  run ./dsh <<EOF
echo hello
rc
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -E "^0$"
}

@test "rc built-in returns error after failing command" {
  run ./dsh <<EOF
non_existing_cmd
rc
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -E "2$"
}

@test "echo with multiple quoted arguments" {
  run ./dsh <<EOF
echo a "b c" d
exit
EOF
  [ "$status" -eq 0 ]
  echo "$output" | grep -q "a b c d"
}
