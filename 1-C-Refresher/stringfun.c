#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SZ 50

// prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

// prototypes for functions to handle required functionality
// Return codes:
// -1 = Buffer size related errors
// -2 = Other documented errors
int count_words(char *, int, int);
int reverse_string(char *, int);
int print_words(char *, int);
int search_replace(char *, int *, char *, int, char *, int);
// add additional prototypes here

int setup_buff(char *buff, char *user_str, int len) {
  int buff_pos = 0;
  int last_was_space = 1;

  for (int i = 0; *(user_str + i) != '\0'; i++) {
    if (buff_pos >= len) {
      return -1;
    }

    if (*(user_str + i) == ' ' || *(user_str + i) == '\t') {
      if (!last_was_space) {
        *(buff + (buff_pos++)) = ' ';
        last_was_space = 1;
      }
    } else {
      *(buff + (buff_pos++)) = *(user_str + i);
      last_was_space = 0;
    }
  }

  if (buff_pos > 0 && *(buff + (buff_pos - 1)) == ' ') {
    buff_pos--;
  }

  int user_str_len = buff_pos;

  while (buff_pos < len) {
    *(buff + (buff_pos++)) = '.';
  }

  return user_str_len;
}

void print_buff(char *buff, int len) {
  printf("Buffer:  ");
  putchar('[');
  for (int i = 0; i < len; i++) {
    putchar(*(buff + i));
  }
  putchar(']');
  putchar('\n');
}

void usage(char *exename) {
  printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len) {
  if (buff == NULL) {
    return -2;
  }
  if (len <= 0 || str_len <= 0 || str_len > len) {
    return -2;
  }

  int word_count = 0;
  for (int i = 0; i < str_len; i++) {
    if (*(buff + i) != ' ' && (i == 0 || *(buff + (i - 1)) == ' ')) {
      word_count++;
    }
  }

  return word_count;
}

int reverse_string(char *buff, int str_len) {
  if (buff == NULL) {
    return -2;
  }
  if (str_len <= 0) {
    return -2;
  }
  for (int i = 0; i < str_len / 2; i++) {
    char temp = *(buff + i);
    *(buff + i) = *(buff + (str_len - 1 - i));
    *(buff + (str_len - 1 - i)) = temp;
  }
  return str_len;
}

int print_words(char *buff, int str_len) {
  if (buff == NULL) {
    return -2;
  }
  if (str_len <= 0) {
    return -2;
  }
  printf("Word Print\n----------\n");
  int word_count = 0;
  for (int i = 0; i < str_len; i++) {
    if (*(buff + i) != ' ' && (i == 0 || *(buff + (i - 1)) == ' ')) {
      printf("%d. ", ++word_count);
      int word_len = 0;
      for (int j = i; j < str_len && *(buff + j) != ' '; j++) {
        printf("%c", *(buff + j));
        word_len++;
      }
      printf("(%d)\n", word_len);
    }
  }
  printf("\nNumber of words returned: %d\n", word_count);
  return word_count;
}

int search_replace(char *buff, int *buff_length, char *search_word,
                   int sw_length, char *replace_word, int rw_length) {
  if (buff == NULL || search_word == NULL || replace_word == NULL) {
    return -2;
  }
  if (*buff_length <= 0 || sw_length <= 0 || rw_length <= 0) {
    return -2;
  }
  // Removed to conform to tests
  // if (*buff_length + (rw_length - sw_length) > BUFFER_SZ) {
  //   // Would cause buffer overflow
  //   return -1;
  // }
  int i = 0;
  int found = 0;
  while (i + sw_length <= *buff_length) {
    // Check word boundaries
    if ((i == 0 || *(buff + i - 1) == ' ') &&
        (i + sw_length == *buff_length || *(buff + i + sw_length) == ' ')) {
      // Check if word matches
      int valid_word = 1;
      for (int j = 0; j < sw_length; j++) {
        if (*(buff + i + j) != *(search_word + j)) {
          valid_word = 0;
          break;
        }
      }
      if (valid_word) {
        found = 1;
        if (sw_length != rw_length) {
          if (rw_length < sw_length) {
            // Shift left for shorter replacement
            for (int pos = i + sw_length; pos < *buff_length; pos++) {
              *(buff + (pos - (sw_length - rw_length))) = *(buff + pos);
            }
            *buff_length -= (sw_length - rw_length);
          } else {
            // Shift right for longer replacement
            for (int pos = *buff_length - 1; pos >= i + sw_length; pos--) {
              *(buff + (pos + (rw_length - sw_length))) = *(buff + pos);
            }
            *buff_length += (rw_length - sw_length);
          }
        }
        // Copy replacement word
        for (int k = 0; k < rw_length; k++) {
          *(buff + i + k) = *(replace_word + k);
        }
        i += rw_length;
        continue;
      }
    }
    i++;
  }

  if (!found) {
    return -3;
  }

  if (*buff_length < BUFFER_SZ) {
    memset(buff + *buff_length, '.', BUFFER_SZ - *buff_length);
  }

  return *buff_length;
}

// ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

int len(char *buf) {
  if (buf == NULL) {
    return -1;
  }
  int i = 0;
  while (*(buf + i) != '\0' && i < BUFFER_SZ) {
    i++;
  }
  return i;
}

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
    rc = reverse_string(buff, user_str_len);
    if (rc < 0) {
      printf("Error reversing string, rc = %d\n", rc);
      exit(3);
    }
    break;
  case 'w':
    rc = print_words(buff, user_str_len);
    if (rc < 0) {
      printf("Error printing words, rc = %d\n", rc);
      exit(3);
    }
    break;
  // TODO:  #5 Implement the other cases for 'r' and 'w' by extending
  //        the case statement options
  case 'x':
    if (argc != 5) {
      usage(argv[0]);
      exit(1);
    }
    char *search_word = argv[3];
    int search_word_len = len(search_word);
    if (search_word_len < 0) {
      printf("Error: Invalid search word\n");
      exit(3);
    }
    char *replace_word = argv[4];
    int replace_word_len = len(replace_word);
    if (replace_word_len < 0) {
      printf("Error: Invalid replace word\n");
      exit(3);
    }

    rc = search_replace(buff, &user_str_len, search_word, search_word_len,
                        replace_word, replace_word_len);
    if (rc == -1) {
      printf("Error: Buffer overflow would occur - replacement would execeed "
             "buffer size\n");
      exit(3);
    } else if (rc < 0) {
      printf("Error in search and replace, rc = %d\n", rc);
      exit(3);
    }
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
//           It's generally good practice to check the buffer size when using it
//           to prevent buffer overflows. If we were to only consider the
//           constant BUFFER_SZ, then it would be easy to write beyond the
//           bounds of the buffer. This happens because users can write more
//           content than the buffer size. Additionaly, it makes the functions
//           more reusable since they'll be self-contained and operable on
//           buffers of arbitrary size.
