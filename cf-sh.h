#ifndef _cfsh_h
#define _cfsh_h

#include <unistd.h>
#include <setjmp.h>

#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define RESET "\x1b[0m"


typedef struct Commands {
    
    char *argv [10];
    pid_t pid;
    int process_killed;
    int do_wait;
    
} * commands;


jmp_buf env;
pid_t glob_pid;
int glob_process_killed;
int glob_do_wait;


struct Commands init ();

void exit_safely (commands, int, int);

void end_child (int);

char *get_path ();

char *newline_to_null (char *);

void parse_input (char *);

int run_cmd (commands);

int piped_process (commands, int);

int cd (char **);

#endif