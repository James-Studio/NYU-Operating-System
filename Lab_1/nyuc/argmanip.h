#ifndef _ARGMANIP_H_
#define _ARGMANIP_H_

char **manipulate_args(int argc, const char *const *argv, int (*const manip)(int));
void free_copied_args(char **args, ...);

#endif