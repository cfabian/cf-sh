#include "cf-sh.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <setjmp.h>


struct Commands init () {
    
    struct Commands cmds;
    int i;
    for (i = 0; i < 10; i ++) {
        
        cmds.argv [i] = NULL;
        
    }
    cmds.process_killed = 0;
    cmds.do_wait = 1;
    
    return cmds;
    
}

void exit_safely (commands cmds, int num_cmds, int msg) {
    /*
    int i;
    
    for (i = 0; i < num_cmds; i ++) {
        
        //printf ("Killing process %d\n", cmds [i].pid);
        kill (cmds [i].pid, SIGKILL);
        cmds[i].process_killed = 1;
        //if (cmds[i].do_wait == 0) { printf ("[1]+ Done\n"); }
        
    }
    */
    if (msg == -1) { exit (EXIT_SUCCESS); }
    
    longjmp (env, 1);
    
}
/*
void end_child (int i) {
    
    kill (glob_pid, SIGKILL);
    glob_process_killed = 1;
    
    signal (SIGCHLD, end_child);
    
    if (glob_do_wait == 0) { printf ("[1]+ Done\n"); longjmp (env, 1); }
    
}
*/
char *get_path () {
    
    char *cwd = malloc (1000 * sizeof (char));
    
    if (getcwd (cwd, 1000) != NULL) {
        
        return cwd;
        
    } else { perror ("jsh: working dir"); exit (EXIT_FAILURE); }
    
}

char *newline_to_null (char *s) {
    
    char *tmp = malloc (1000 * sizeof (char));
    
    tmp = strchr (s, '\n');
    if (tmp != NULL) {
        
        s [tmp - s] = '\0';
        
    }
    
    return s;
    
}

void parse_input (char *cmd_line_input) {
    
    int i = 0, len, exit_status, num_cmds = 1, run_piped = 0;
    char *buffer;
    
    int stdinfd, stdoutfd;
    int fd;
    
    commands cmds = malloc (sizeof (struct Commands));
    cmds [num_cmds - 1] = init ();
    
    if ((stdinfd = dup (0)) < 0) { perror ("jsh: dup (stdin)"); exit (EXIT_FAILURE); }
    if ((stdoutfd = dup (1)) < 0) { perror ("jsh: dup (stdout)"); close (stdinfd); exit (EXIT_FAILURE); }
    
    //fprintf (stderr, "FD %d and FD %d\n", stdinfd, stdoutfd);
    
    if (strcmp (cmd_line_input, "exit") == 0) { close (stdinfd); close (stdoutfd); exit_safely (cmds, num_cmds, -1); }
    
    buffer = strtok (cmd_line_input, " ");
    
    while (buffer != NULL) {
        
        if (strcmp (buffer, "&") == 0) {
            
            cmds [num_cmds - 1].do_wait = 0;
            buffer = strtok (NULL, " ");
            
        } else if (strcmp (buffer, "<") == 0) {
            
            //printf ("< detected\n");
            buffer = strtok (NULL, " ");
            fd = open (buffer, O_RDONLY | O_CREAT, 0644);
            
            if (dup2 (fd, 0) < 0) { perror ("jsh: dup2 <"); exit (EXIT_FAILURE); }
            
            close (fd);
            
            buffer = strtok (NULL, " ");
            
        } else if (strcmp (buffer, ">") == 0) {
            
            //printf ("> detected\n");
            buffer = strtok (NULL, " ");
            fd = open (buffer, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (dup2 (fd, 1) < 0) { perror ("jsh: dup2 >"); exit (EXIT_FAILURE); }
            
            close (fd);
            fflush (stdout);
            
            buffer = strtok (NULL, " ");
            
        } else if (strcmp (buffer, ">>") == 0) {
            
            //printf (">> detected\n");
            buffer = strtok (NULL, " ");
            fd = open (buffer, O_WRONLY | O_CREAT | O_APPEND, 0644);
            
            if (dup2 (fd, 1) < 0) { perror ("jsh: dup2 >>"); exit (EXIT_FAILURE); }
            
            close (fd);
            fflush (stdout);
            
            buffer = strtok (NULL, " ");
            
        } else if (strcmp (buffer, "|") == 0) {
            
            run_piped = 1;
            cmds [num_cmds - 1].argv [i] = NULL;
            num_cmds ++;
            cmds = realloc (cmds, num_cmds * sizeof (struct Commands));
            cmds [num_cmds - 1] = init ();
            buffer = strtok (NULL, " ");
            i = 0;
            
        } else {
            
            len = strlen (buffer);
            
            cmds [num_cmds - 1].argv [i] = malloc ((len + 1) * sizeof (char));
            strncpy (cmds [num_cmds - 1].argv [i], buffer, len);
            cmds [num_cmds - 1].argv [i] [len] = '\0';
            //printf (YELLOW "%d %s\n" RESET, len, cmds [num_cmds - 1].argv [i]);
            i ++;
        
            buffer = strtok (NULL, " ");
        
        }
        
    }
    
    cmds [num_cmds - 1].argv [i] = NULL;
    
    if (strcmp (cmds [num_cmds - 1].argv [0], "cd") == 0) {
        
        cd (cmds [num_cmds - 1].argv);
        
    } else if (run_piped == 1) { 
        
        piped_process(cmds, num_cmds);
        
    }else {
        
        //fprintf (stderr, YELLOW "%s\n" RESET, cmds [0].argv [0]);
        exit_status = run_cmd (cmds);
        
    }
    
    if (exit_status == 1) { }//fprintf (stderr, "jsh: %s: command not found\n", cmds [0].argv [0]); }
    
    if (dup2 (stdinfd, 0) < 0) { if (dup2 (fd, 0) < 0) { perror ("jsh: dup2 (stdin)"); exit (EXIT_FAILURE); } }
    if (dup2 (stdoutfd, 1) < 0) { if (dup2 (fd, 1) < 0) { perror ("jsh: dup2 (stdout)"); exit (EXIT_FAILURE); } }
    
    //fprintf (stderr, YELLOW "%d\n" RESET, fd);
    if (fd != 0 && fd != 1) { close (fd); }
    //fprintf (stderr, YELLOW "stdinfd: %d, stdoutfd: %d\n" RESET, stdinfd, stdoutfd);
    while (close (stdinfd) != 0) { }
    while (close (stdoutfd) != 0) { }
    
    //printf ("exit status: %d\n", exit_status);
    
    return;
    
}

int run_cmd (commands cmds) {
    
    int wait_status;
    
    pid_t w;
        
    glob_pid = cmds [0].pid = fork ();
    
    if (cmds [0].pid == -1) { perror ("fork"); exit (EXIT_FAILURE); }
    else if (cmds [0].pid == 0) { //Child
            
        //if (cmds [0].do_wait == 0) { printf ("\n[1] %d\n", getpid ()); }
        close (3);
        close (4);
        if (execvp (cmds [0].argv [0], cmds [0].argv) == -1) { perror (cmds [0].argv [0]); exit (EXIT_FAILURE); }
        
    } else { //Parent
    
        if (cmds [0].do_wait == 1) {
            
            while ((w = wait (&wait_status)) != cmds [0].pid);
            
            if (w == -1 && cmds [0].process_killed == 0) { perror ("wait"); exit_safely (cmds, 1, 0); return -1; }
            else {
                
                if (WIFEXITED (wait_status)) {
                    
                    return WEXITSTATUS (wait_status);
                    
                } else if (cmds [0].process_killed == 0) { perror ("wait"); exit_safely (cmds, 1, 0); return -1; }
            }
            
        } else {
            
            return 0;
            
        }
    }
    
    return -1;
    
}

int piped_process (commands cmds, int num_cmds) {
    
    int num_pipes = num_cmds - 1;
    int * pipes[num_pipes];
    
    int i, j ,k;
    
    for (i = 0; i < num_pipes; i ++) {
        
        pipes [i] = malloc (sizeof (int) * 2);
        pipe (pipes [i] );
        
    }
    
    pid_t pid;
    int c_pipe = 0;
    
    for (i = 0; i < num_cmds; i ++) {
    
        pid = fork();
        
        cmds [i].pid = pid;
        if (pid == 0) { // Child
            
            int fd;
        
            if (i == 0) { //First 
            
                close (pipes [c_pipe] [0]);
                
            } else {
                    
                if (i == num_cmds - 1) {
                    
                    if (dup2 (pipes [c_pipe] [0], 0) < 0) { perror (cmds [i].argv [0]); }
                    
                } else {
                        
                    if (dup2 (pipes [c_pipe] [0], 0) < 0) { perror (cmds [i].argv [0]); }
                }
            }
            
            if (i == num_cmds - 1) { //Last
                
                close (pipes [c_pipe] [1]);
                    
            } else {
                
                if (i == 0) {
                    
                    if (dup2 (pipes [c_pipe] [1], 1) < 0) { perror (cmds [i].argv [0]); }
                    
                } else {
                        
                    if (dup2 (pipes [c_pipe + 1] [1], 1) < 0) { perror(cmds[i].argv[0]); }
                }
            }
                
            for (j = 0; cmds [i].argv [j] != NULL; j ++) {
                
                char *buffer = cmds [i].argv [j] == NULL ? NULL : malloc (strlen (cmds [i].argv [j]) + 1);
                if (buffer == NULL){ continue; }
                strcpy (buffer, cmds [i].argv [j]);
                
                if (strcmp (buffer, "<") == 0) {
                    
                    cmds [i].argv [j] = NULL;
                    fd = open (cmds [i].argv [j + 1], O_RDONLY | O_CREAT, 0644);
                    cmds [i].argv [j + 1] = NULL;
                    if (dup2 (fd, 0) < 0) { perror ("jsh: <"); exit (EXIT_FAILURE); }
                    
                    close (fd);
                    
                } else if (strcmp (buffer, ">") == 0) {
                    
                    cmds [i].argv [j] = NULL;
                    fd = open (cmds [i].argv [j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    cmds [i].argv [j + 1] = NULL;
                
                    if (dup2 (fd, 1) < 0) { perror ("jsh: >"); exit (EXIT_FAILURE); }
                    
                    close (fd);
                    fflush (stdout);
                    
                } else if (strcmp (buffer, ">>") == 0) {
                    
                    cmds [i].argv [j] = NULL;
                    fd = open (cmds [i].argv [j + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    cmds [i].argv [j + 1] = NULL;
                    if (dup2 (fd, 1) < 0) { perror ("jsh: >>"); exit (EXIT_FAILURE); }
                    
                    close (fd);
                    fflush (stdout);
                    
                }
            }
            
            for (k = 3; k < 1000; k ++) {
                
                close (k);
                
            }
            
            //fprintf (stderr, "Runnning %s\n", cmds [i].argv [0]);
            int status = execvp (cmds [i].argv [0], cmds [i].argv);
             
            if (status == -1) {
                
                perror (cmds [i].argv [0]);
                exit (EXIT_FAILURE);
                
            }
            
            exit (0);
            
        } else { // Parent
            
            if (i != 0) {
                
                c_pipe ++;
            
            }
        }
    }
    
    for (i = 0; i < num_pipes; i ++) {
        
        close (pipes [i] [0]);
        close (pipes [i] [1]);
        
    }
    
    pid_t wa_pid;
    int wa_stat;
    int have_seen = 0;
    
    while ((wa_pid = wait (&wa_stat))) {
        
        have_seen ++;
        if (have_seen == num_cmds) {
            
            break;
            
        }
    }
    
    if (wa_pid == -1) {
        
        perror ("jsh: wait");
        exit (EXIT_FAILURE);
        
    } else {
        
        if(WIFEXITED (wa_pid) == 0) {
            
            return (WIFEXITED (wa_pid));
            
        } else {
            
            perror ("jsh: WIFEXITED");
            exit (EXIT_FAILURE);
            
        }
    }
    
    return 0;
   
}




/*
int piped_process (commands cmds, int num_cmds) {
    
    int oddfd [2], evenfd [2];
    int i;
    
    int wait_status;
    pid_t w;
    
    for (i = 0; i < num_cmds; i ++) {
        
        pipe (oddfd);
        pipe (evenfd);
        
        //fprintf (stderr, "%d/%d - %s\n", i, num_cmds, cmds [i].argv [0]);
        
        cmds [i].pid = fork ();
        
        if (cmds [i].pid == -1) { 
            
            perror ("fork");
            if (i % 2 != 0) {
                
                close (oddfd [1]);
                
            } else {
                
                close (evenfd [1]);
                
            }
            exit (EXIT_FAILURE); 
            
        } else if (cmds [i].pid == 0) { //Child
            
            if (i == 0) {
                
                if (dup2 (evenfd [1], 1) < 0) { perror ("jsh: dup2 (pipe stdout):"); exit (EXIT_FAILURE); }
                close (evenfd [0]);
                
            } else if (i == (num_cmds -1)) {
                
                if (i % 2 == 0) {
                    
                    if (dup2 (oddfd [0], 0) < 0) { perror (">jsh: dup2 (pipe stdin):"); exit (EXIT_FAILURE); }
                    close (oddfd [1]);
                    
                } else {
                    
                    if (dup2 (evenfd [0], 0) < 0) { perror ("jsh: dup2 (pipe stdin):"); exit (EXIT_FAILURE); }
                    close (evenfd [1]);
                    
                }   
                
            } else {
                
                if (i % 2 == 0) {
                    
                    if (dup2 (oddfd [0], 0) < 0) { perror ("jsh: dup2 (pipe stdin):"); exit (EXIT_FAILURE); }
                    if (dup2 (evenfd [1], 1) < 0) { perror ("jsh: dup2 (pipe stdout):"); exit (EXIT_FAILURE); }
                    
                } else {
                    
                    if (dup2 (evenfd [0], 0) < 0) { perror ("jsh: dup2 (pipe stdin):"); exit (EXIT_FAILURE); }
                    if (dup2 (oddfd [1], 1) < 0) { perror ("jsh: dup2 (pipe stdout):"); exit (EXIT_FAILURE); }
                    
                }
                
            }
            
            //fprintf (stderr, "Running %s\n", cmds [i].argv [0]);
            close (3);
            close (4);
            if (execvp (cmds [i].argv [0], cmds [i].argv) == -1) { perror (cmds [i].argv [0]); exit (EXIT_FAILURE); }
        
        } else { //Parent
        
            if (i == 0) {
                
                close (evenfd [1]);
                
            } else if (i == (num_cmds - 1)) {
                
                if (i % 2 != 0) {
                    
                    close (oddfd [0]);
                    
                } else {
                    
                    close (evenfd [0]);
                    
                }
                
            } else {
                
                if (i % 2 != 0) {
                    
                    close (evenfd [0]);
                    close (oddfd [1]);
                    
                } else {
                    
                    close (oddfd [0]);
                    close (evenfd [1]);
                    
                }
            }
        }
    }
    
    close (oddfd [0]);
    close (oddfd [1]);
    close (evenfd [0]);
    close (evenfd [1]);
    
    for (i = 0; i < num_cmds; i ++) {
        
        if (cmds [i].do_wait == 1) {
                    
            while ((w = wait (&wait_status)) != cmds [i].pid);
                    
            if (w == -1 && cmds [i].process_killed == 0) { perror ("wait"); exit_safely (cmds, 1, 0); return -1; }
            else {
                        
                if (WIFEXITED (wait_status)) {
                            
                    //return WEXITSTATUS (wait_status);
                            
                } else if (cmds [i].process_killed == 0) { perror ("wait"); exit_safely (cmds, 1, 0); return -1; }
            }
                    
        } else {
                    
            //return 0;
                    
        }
        
    }
    
    return 0;
    
}
*/
int cd (char *argv []) {
    
    int err;
    
    if (argv [1] == NULL) {
        
        return -1;
        
    } else {
        
        err = chdir (argv [1]);
        
        if (err != 0) { perror ("chdir"); return -1; }
        else { return 0; }
    }
}

