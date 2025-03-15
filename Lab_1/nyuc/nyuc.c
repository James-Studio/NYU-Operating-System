#include <ctype.h>
#include <stdio.h>

#include "argmanip.h"

int main(int argc, const char *const *argv) {
  char **upper_args = manipulate_args(argc, argv, toupper);
  char **lower_args = manipulate_args(argc, argv, tolower);

  for (char *const *p = upper_args, *const *q = lower_args; *p && *q; ++argv, ++p, ++q) {
    printf("[%s] -> [%s] [%s]\n", *argv, *p, *q);
  }
  free_copied_args(upper_args, lower_args, NULL);
}
