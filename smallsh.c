// Ethan Blake
// Assignment 3: smallsh
// due: 2/7/22


// include statements
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include "get_user_input.c"


// Macros
#define MAX_LENGTH 2048
#define MAX_ARGS 512


// Global variables
int childStatus = 0;
int fg_only_flag = 0;           // 1 means (ignore &) as part of commands


/* signal handler for SIGTSTP */
void SIGTSTP_handler(int signo){
    // This handler handles CTRL z witch changes the foreground mode by using a global flag variable. 
    if (fg_only_flag == 0){
        write(STDOUT_FILENO, "\nEntering foreground-only mode (& is now ignored)\n: " , 52);
        fg_only_flag = 1;

    }
    else {
        write(STDOUT_FILENO, "\nExiting foreground-only mode\n: " , 32);
        fg_only_flag = 0;
    }

}


// The main function
// This function calls get_user_input(user_input) which returns a parsed array of strings. it then parses the 
// the user input a little bit more, and then runs either one of the build in commands or forks off a child process
// either in the foreground or background from the exec family of functions.
// Additionally, this function uses some signal handlers and wait pid to check ctrl z, ctrl c, and if there are any 
// rogue processes. 
int main (void){

    // for the signal handlers I use the basic structure given to us in the modules, except I used SA_RESTART instead of 0 
    // to prevent some weird issues I got when using 0.
    // SIGINT signal handler
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;        // this means to ignore the signal
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    // SIGTSTP signal handler
    struct sigaction sa2;
    sa2.sa_handler = SIGTSTP_handler;        // this calls SIGTSTP handler
    sigfillset(&sa2.sa_mask);
    sa2.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa2, NULL);

    // user input while loop will run until exit command is run
    while (1){
        // check if there are any terminated processes
        pid_t pid;
        while((pid = waitpid(-1, &childStatus, WNOHANG)) > 0){
            printf("background pid %d is done: ", pid);
            if (WIFEXITED(childStatus)) {
		    // If exited by status
		        printf("exit value %d\n", WEXITSTATUS(childStatus));
	        } else {
            // If terminated by signal
		    printf("terminated by signal %d\n", WTERMSIG(childStatus));
	        }
        }

        // get user input
        char** arr = user_input();

        // comment/nothing typed functionality
        if (!(strcmp(arr[1], ""))){
            continue;
        }
        if (!(strcmp(arr[1], "&"))){
            printf("&: no such file or directory\n");
            continue;
        }
        // I was having gigantic troubles with the formatting of my array, so I chose to just make a new array from the old one.
        char *newargv[MAX_ARGS];
        int size = atoi(arr[0]);        // I shipped over the size of the array in the first element, probably not my brightest decision, but it works.
        for(int i = 1; i <= size; i++){
            newargv[i-1] = arr[i];
            newargv[i] = NULL;
        }

        // Here I just initialize some things
        int file_descriptor;
        int close_fd_flag = 0;
        int bg_flag = 0;                // 0 means run in foreground, 1 means run in background
        
        // enter foreground only mode if flag == 1
        if (fg_only_flag == 1){
            bg_flag = 0;
            if ((strcmp(newargv[size-1], "&") == 0)){
                newargv[size-1][0] = '0';               // get rid of the & command and replace it with a 0
            }
        }

        // check if needs to be run in the foreground or the background. if 0, then foreground, if 1, then background.
        if ((strcmp(newargv[size-1], "&") == 0) && fg_only_flag == 0){
            bg_flag = 1;
            newargv[size-1][0] = '0';
        }

        // exit functionality
        if (!(strcmp(newargv[0], "exit"))){
            kill(0, SIGKILL);               // I used 0 SIGKILL so I don't have to keep track of every process I've created
            return EXIT_SUCCESS;
        }

        // cd functionallity
        // if there is only one argument, then go to the home directory
        else if ((strcmp(newargv[0], "cd") == 0) && arr[2][0]== '\0'){      
            chdir("/nfs/stak/users/blakeet");
        } 
        // otherwise concatenate everything together and make that the current directory
        else if (!(strcmp(newargv[0], "cd"))){
            char s[100];
            char* new_dir;
            char* fin_dir;
            char* slash = "/";
            if (newargv[1][0] == '/'){              // if it starts with / then we need to start concatenating from the base directory
                fin_dir = newargv[1];
            } else {
                new_dir = strcat(getcwd(s, 100),"/");
                fin_dir = strcat(new_dir,newargv[1]);
            }
            chdir(fin_dir);
        }

        // status functionality this is pretty basic
        else if (!(strcmp(newargv[0], "status"))){
            printf("exit value %d\n", WEXITSTATUS(childStatus));
            fflush(stdout);
        }   
        // close_fd_flagion functionality, loop through and check for <>, as well as execution functionality
        else {
            childStatus;
            pid_t spawnPid = fork();
            switch(spawnPid){
                case -1:
                    // if there is an error forking
                    perror("fork()\n");
                    exit(1);
                case 0:
                    // child process
                    // this part sets the signal handler to ignore
                    sa2.sa_handler = SIG_IGN;      
                    sigaction(SIGTSTP, &sa2, NULL);

                    if (bg_flag== 0){
                        sa.sa_handler = SIG_DFL;        // this sets the signal handler to unignore
                        sigaction(SIGINT, &sa, NULL);
                    }
                    // if file not specified, open /dev/null
                    if (bg_flag == 1) {	                        // if we are in the background, we want to set the file_descriptor to /dev/null
                        close_fd_flag = 1;
                        file_descriptor = open("/dev/null", O_RDONLY);
                        int check = dup2(file_descriptor, STDIN_FILENO);    // this is just to check if the process failed
                        if (check == -1) { 
                            perror("error close_fd_flaging to /dev/null"); 
                            exit(1); 
                        }
                    }
                    // if the file is specified, open the specified file
                    for(int i = 0; i <= size-1; i++){
                        // here I go through and check to find if we need to open a source file or open an output file
                        if (newargv[i][0] == '<'){
                            // open input file
                            close_fd_flag = 1;
                            file_descriptor = open(newargv[i+1], O_RDONLY);
                            int check = dup2(file_descriptor, STDIN_FILENO);
                            if (check == -1) { 
                                printf("cannot open %s for input\n", newargv[i+1]); 
                                exit(1); 
                            }
                        }
                        if (newargv[i][0] == '>'){
                            // Open output file
                            close_fd_flag = 1;  
                            file_descriptor = open(newargv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            int check = dup2(file_descriptor, STDOUT_FILENO);
                            if (check == -1) { 
                                printf("cannot open %s for output\n", newargv[i+1]); 
                                exit(1);  
                            }   
                        }
                    }
                    // this is to close the file desciptor
                    if (close_fd_flag == 1){
                        close(file_descriptor);
                        int remove_brackets_flag = 0;
                        // this is to remove all elements after the < or > file because we have already set them
                        for (int i=1; i<size; i++){
                            if (newargv[i][0] == '<' || newargv[i][0] == '>'){
                                remove_brackets_flag = 1;
                            }
                            if (remove_brackets_flag == 1){
                                newargv[i] = NULL;
                            }
                        }
                    }
                    // here we run execvp
                    execvp(newargv[0], newargv);
                    perror(newargv[0]);
                    exit(1);
                default:
                    // parent process
                    if (bg_flag == 1){
                        printf("background pid is %d\n", spawnPid);
                        // WNOHANG means that it will let things continue in the background
                        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
                        fflush(stdout);
                    }
                    else {
                        // 0 instead of WNOHANG means that we wait until the child process is finished
                        pid_t actualPid = waitpid(spawnPid, &childStatus, 0);
                        // this is to check if the process is terminated by ctrl c
                        if (WIFSIGNALED(childStatus) == 1 && WTERMSIG(childStatus) != 0){
                            printf("terminated by signal %d\n", WTERMSIG(childStatus));
                            fflush(stdout);
                        }
                    }
                }  
        }
    }
}
