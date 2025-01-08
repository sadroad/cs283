#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SZ 50

// prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

// prototypes for functions to handle required functionality
// TODO: replace void with int and error codes
int count_words(char *, int, int);
void print_reverse_string(char *, int);
void print_words(char *, int);
void search_replace(char *, int, char *, int);
// add additional prototypes here

int setup_buff(char *buff, char *user_str, int len) {
  int buff_pos = 0;
  int last_was_space = 1;

  for (int i = 0; user_str[i] != '\0'; i++) {
    if (buff_pos >= len) {
      return -1;
    }

    if (user_str[i] == ' ' || user_str[i] == '\t') {
      if (!last_was_space) {
        buff[buff_pos++] = ' ';
        last_was_space = 1;
      }
    } else {
      buff[buff_pos++] = user_str[i];
      last_was_space = 0;
    }
  }

  if (buff_pos > 0 && buff[buff_pos - 1] == ' ') {
    buff_pos--;
  }

  int user_str_len = buff_pos;

  while (buff_pos < len) {
    buff[buff_pos++] = '.';
  }

  return user_str_len;
}

void print_buff(char *buff, int len) {
  printf("Buffer:  ");
  for (int i = 0; i < len; i++) {
    putchar(*(buff + i));
  }
  putchar('\n');
}

void usage(char *exename) {
  printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len) {
  int word_count = 0;
  for (int i = 0; i < str_len; i++) {
    if (buff[i] != ' ' && (i == 0 || buff[i - 1] == ' ')) {
      word_count++;
    }
  }

  return word_count;
}

void print_reverse_string(char *buff, int str_len) {
  printf("Reveresed String: ");
  for (int i = str_len - 1; i >= 0; i--) {
    printf("%c", buff[i]);
  }
  printf("\n");
}

void print_words(char *buff, int str_len) {
  printf("Word Print\n----------\n");
  int word_count = 0;
  for (int i = 0; i < str_len; i++) {
    if (buff[i] != ' ' && (i == 0 || buff[i - 1] == ' ')) {
      printf("%d. ", ++word_count);
      int word_len = 0;
      for (int j = i; j < str_len && buff[j] != ' '; j++) {
        printf("%c", buff[j]);
        word_len++;
      }
      printf(" (%d)\n", word_len);
    }
  }
}

// ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

int main(int argc, char *argv[]) {

  char *buff;         // placehoder for the internal buffer
  char *input_string; // holds the string provided by the user on cmd line
  char opt;           // used to capture user option from cmd line
  int rc;             // used for return codes
  int user_str_len;   // length of user supplied string

  // TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
  /*
   * The following if statement checks that there are at least 2 arguments
   * when the program is called.
   * argv[1] is always defined when argc >= 2 and if there were less than 2
   * arguments, the if statement would end execution before reading the value
   * of an uninitialized argv[1]
   *
   */
  if ((argc < 2) || (*argv[1] != '-')) {
    usage(argv[0]);
    exit(1);
  }

  opt = (char)*(argv[1] + 1); // get the option flag

  // handle the help flag and then exit normally
  if (opt == 'h') {
    usage(argv[0]);
    exit(0);
  }

  // WE NOW WILL HANDLE THE REQUIRED OPERATIONS

  // TODO:  #2 Document the purpose of the if statement below
  //  The if statement checks that there is a string to operate on
  if (argc < 3) {
    usage(argv[0]);
    exit(1);
  }

  input_string = argv[2]; // capture the user input string

  // TODO:  #3 Allocate space for the buffer using malloc and
  //           handle error if malloc fails by exiting with a
  //           return code of 99
  buff = malloc((sizeof *buff) * BUFFER_SZ);
  if (buff == NULL) {
    exit(99);
  }

  user_str_len = setup_buff(buff, input_string, BUFFER_SZ); // see todos
  if (user_str_len < 0) {
    printf("Error setting up buffer, error = %d", user_str_len);
    exit(2);
  }

  switch (opt) {
  case 'c':
    rc = count_words(buff, BUFFER_SZ, user_str_len); // you need to implement
    if (rc < 0) {
      printf("Error counting words, rc = %d", rc);
      exit(2);
    }
    printf("Word Count: %d\n", rc);
    break;
  case 'r':
    print_reverse_string(buff, user_str_len);
    break;
  case 'w':
    print_words(buff, user_str_len);
    break;
  // TODO:  #5 Implement the other cases for 'r' and 'w' by extending
  //        the case statement options
  case 'x':
    if (argc == 5) {
      usage(argv[0]);
      exit(1);
    }
    search_replace(buff, user_str_len, buff, user_str_len);
    break;
  default:
    usage(argv[0]);
    exit(1);
  }

  // TODO:  #6 Dont forget to free your buffer before exiting
  print_buff(buff, BUFFER_SZ);
  free(buff);
  exit(0);
}

// TODO:  #7  Notice all of the helper functions provided in the
//           starter take both the buffer as well as the length.  Why
//           do you think providing both the pointer and the length
//           is a good practice, after all we know from main() that
//           the buff variable will have exactly 50 bytes?
//
//           PLACE YOUR ANSWER HERE
