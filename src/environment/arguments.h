#ifndef ARGUMENTS_H
#define ARGUMENTS_H

void init_main_argv(char **argv, int offset);

char **get_current_args(void);

void set_current_args(char **new_args);

int get_argc(void);

#endif /* ! ARGUMENTS_H */
