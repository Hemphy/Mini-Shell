/* Author(s): Dennis Dang
 *
 * This is lab9.c the csc60mshell
 * This program serves as a skeleton for doing labs 9, 10, 11.
 * Student is required to use this program to build a mini shell
 * using the specification as documented in direction.
 * Date: April 26, Spring 2018
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAXLINE 80
#define MAXARGS 20
#define MAX_PATH_LENGTH 50
#define TRUE 1

/* function prototypes */
int parseline(char *cmdline, char **argv);

void process_input(int argc, char **argv); 
void handle_redir(int count, char *argv[]); 

/* ----------------------------------------------------------------- */
/*                  The main program starts here                     */
/* ----------------------------------------------------------------- */
int main(void)
{
    char cmdline[MAXLINE];
    char *argv[MAXARGS];
    int argc;
    int status;
    pid_t pid;

    /* Loop forever to wait and process commands */
    while (TRUE) 
    {
	/* Print your shell name: csc60mshell (m for mini shell) */
	printf("csc60mshell> ");

	/* Read the command line */
	fgets(cmdline, MAXLINE, stdin);

	/* Call parseline to build argc/argv */
        argc = parseline(cmdline, argv);

        printf("Argc = %i\n", argc);
        
        int i;
        for(i = 0; i < argc; i++)
        {
            printf("Argv %i = %s \n", i, argv[i]);
        }
        
        if(argc == 0)
            continue;

        if(strcmp(argv[0], "exit") == 0)
        {
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argv[0], "pwd") == 0)
        {
            char path[MAX_PATH_LENGTH];
            getcwd(path, MAX_PATH_LENGTH);
            printf("%s\n", path);
            continue;
        }
        else if(strcmp(argv[0], "cd") == 0)
        {
            char *dir;
            if (argc == 1)
                dir = getenv("HOME");
            else
                dir = argv[1];
            if (chdir(dir) == -1)
                perror("Error changing directory");
            continue;
        }
	/* Else, fork off a process */
        else 
        {
            pid = fork();
            switch(pid)
            {
	    	case -1:
                perror("Shell Program fork error");
	            exit(EXIT_FAILURE);
	   	case 0:
		    /* I am child process. I will execute the command, */
		    /* and call: execvp */
		    process_input(argc, argv);
		    break;
	   	default:
		    /* I am parent process */
		    if (wait(&status) == -1)
		    	perror("Parent Process error");
		    else
		    	printf("Child returned status: %d\n",status);
		    break;
	   }	/* end of the switch */
	}	/* end of the if-else-if */
    }		/* end of the while */
} 		/* end of main */

/* ----------------------------------------------------------------- */
/*                  parseline                                        */
/* ----------------------------------------------------------------- */
/* parse input line into argc/argv format */

int parseline(char *cmdline, char **argv)
{
    int count = 0;
    char *separator = " \n\t"; /* Includes space, Enter, Tab */
 
    /* strtok searches for the characters listed in separator */
    argv[count] = strtok(cmdline, separator);

    while ((argv[count] != NULL) && (count+1 < MAXARGS)) 
    	argv[++count] = strtok((char *) 0, separator);
     		
    return count;
}

/* ----------------------------------------------------------------- */
/*                  process_input                                    */
/* ----------------------------------------------------------------- */
void process_input(int argc, char **argv) 
{                       
    /* Step 1: Call handle_redir to deal with operators:            */
    /* < , or  >, or both                                           */
    handle_redir(argc, argv);

    /* Step 2: perform system call execvp to execute command        */
    /* Hint: Please be sure to review execvp.c sample program       */
    int returned_value = execvp(argv[0], argv);
    if (returned_value  == -1)
    {                                        
        fprintf(stderr, "Error on the exec call\n");              
        _exit(EXIT_FAILURE);                                      
    }                                                            
 
 }

/* ----------------------------------------------------------------- */
/*                  handle_redir                                     */
/* ----------------------------------------------------------------- */
void handle_redir(int count, char *argv[])
{
    int out_redir = 0;
    int in_redir = 0;
    int i;
    for (i = 0; i < count; i++)
    {
        if (strcmp(argv[i], ">") == 0)
        {
            if (out_redir != 0)
            {
                // Cannot output more than one file. print error. _exit failure
                fprintf(stderr, "Cannot output more than one file.\n");
                _exit(EXIT_FAILURE);
            }   
            else if (i == 0)
            {
                // No command entered. print error. _exit failure 
                fprintf(stderr, "No command entered.\n");
                _exit(EXIT_FAILURE);
            }
            out_redir = i; 
        }
        else if (strcmp(argv[i], "<") == 0)
        {
            if (in_redir != 0)
            {
                // Cannot output more than one file. print error. _exitfailure
                fprintf(stderr, "Cannot output more than one file.\n");
                _exit(EXIT_FAILURE);
            }
            else if (i == 0)
            {
                // No command entered. prent error. _exit failure
                fprintf(stderr, "No command entered.\n");
                _exit(EXIT_FAILURE);
            }
            in_redir = i;
        }
    }

    if (out_redir != 0)
    {
        if (argv[out_redir + 1] == NULL)
        {
            // There is no file, so print an error and _exit in failure.
            fprintf(stderr, "No file");
            _exit(EXIT_FAILURE);
        }
        
        // Open the file using name from argv, indexed out_redir+1,
        //      and assign returned value to fd. [See 9-Unix, slides 6-10]
        //      use flags: to write; to create file if needed; to truncate existing file to zero length
        //      use permision bits for: user-read; user-write;
        // Error Check the open. _exit
        int fd = open(argv[out_redir + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd == -1)
        {
            perror("Error openning file");
            _exit(EXIT_FAILURE);
        }
        // call dup2 to switch std-out to the value of the file descriptor.
        // Close the file
        dup2(fd, 1);
        close(fd);

        // Set things up for the future exec call by setting argv[out_redir] to NULL
        argv[out_redir] = NULL;
    }

    if (in_redir != 0)
    {
        if (argv[out_redir + 1] == NULL)
        {
            // There is no file, so print an error, and _exit in failure.
            fprintf(stderr, "No file");
            _exit(EXIT_FAILURE);
        }
        // Open the file using name from argv, indexed out by in_redir+1
        //      and assign returned value to fd. [See 9-Unix, slides 6-10]
        //      use flags; for read only
        // Error check the open. _exit
        int fd = open(argv[in_redir + 1], O_RDONLY);
        if (fd == -1)
        {
            perror("Error on openning file");
            _exit(EXIT_FAILURE);
        }
        
        // Call dup2 to switch standard-in to the value of the file descriptor
        // Close the file
        dup2(fd, 0);
        close(fd);
        
        // Set things up for the future exec call by setting argv[in_redir] to NULL
        argv[in_redir] = NULL;
    }
}
