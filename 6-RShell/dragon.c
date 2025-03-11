#include <stdio.h>

extern char _binary_dragon_bin_start[];
extern char _binary_dragon_bin_end[];

void print_dragon() {
  size_t dragon_size = _binary_dragon_bin_end - _binary_dragon_bin_start;

  printf("%.*s\n", (int)dragon_size, _binary_dragon_bin_start);
}
