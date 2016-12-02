#include "cf-sh.h"
#include "cf-sh.c"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>


int main (int argc, char *argv[]) {
    
    //signal (SIGCHLD, end_child);
    
    int input_size = (1000 * sizeof (char));
    char *cmd_line_input = malloc (input_size);
    
    while (1) {
        
        setjmp (env);
        
        if (argc == 1) {
            
            printf (RED "cf-sh:" RESET YELLOW "%s " RESET "$ ", get_path ());
        
        } else if (argc == 2) {
            
            if (strcmp (argv[1], "-") != 0) {
                
                printf (RED "%s:" RESET YELLOW "%s " RESET "$ ", argv[1], get_path ());
                
            }
            
        } else { perror ("cf-sh: argc"); exit (EXIT_FAILURE); }
            
        if (fgets (cmd_line_input, input_size, stdin)) {
            
            if (cmd_line_input [0] != '\n') {
                
                cmd_line_input = newline_to_null (cmd_line_input);
                
                parse_input (cmd_line_input);
                
            }
            
            cmd_line_input = malloc (input_size);
        
        } else {
            
            return 0;
            
        }
        
    }
    
    return 0;
    
}