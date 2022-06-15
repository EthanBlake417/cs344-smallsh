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

// Macros
#define MAX_LENGTH 2048
#define MAX_ARGS 512


// Gets the user input and returns an array of strings. 
// Additionally does some of the parsing necessary for the requirements of the project.
// The first element of the array is the size of the array
char** user_input(void){

    // Create an array to hold the space delimeted strings
    // here I use MAX_LENGTH and MAX_ARGS so that things won't be too big or too small.
    char ** arr = malloc(MAX_ARGS*sizeof(char*));
    for (int i =0 ; i < MAX_ARGS; ++i)
        arr[i] = malloc(20 * sizeof(char));

    // Create a buffer to hold user input
    char buffer[MAX_LENGTH];
    char *input = buffer;
    size_t len = MAX_LENGTH;
    size_t num_char;
    // prints the colon where the user will enter what they want
    printf(": ");
    fflush(stdout);
    // Here I just want to remove the newline character from the string
    num_char = getline(&input,&len,stdin) - 1;
    // in the case that there is an #, we just want o return an empty array.
    if (strncmp(input, "#", 1) == 0){
        return (arr);
    }

    // here we make a string with the pid number
    char pid_string[20];
    sprintf(pid_string, "%d", getpid());
    // here we create a dollar string
    char dollar[5];
    strcpy(dollar, "$");

    // initialize some things    
    int dollar_counter = 0;
    int size = 1;
    int i = 0;
    // this while loop is to handle all the situations with dollar signs, so that two in a row creates the smallsh id.
    while(input[i] != '\n'){
        if (input[i] == '$' && dollar_counter == 0){
            dollar_counter = 1;
        } else if (input[i] == '$' && dollar_counter == 1){
            strncat(arr[size], pid_string, strlen(pid_string));
            dollar_counter = 0;
        } else if (dollar_counter == 1){
            strncat(arr[size], dollar, 1);
            strncat(arr[size], &input[i], 1);
            dollar_counter = 0;
        } else if (input[i] != ' '){
            strncat(arr[size], &input[i], 1);
        }
        if (input[i] == ' ' && arr[size][0] != '\0'){
            size ++;
        }
        i++;
    }
    // edge case
    if (dollar_counter == 1){
        strncat(arr[size], dollar, 1);
    }

    // append size to the beginning of the array, once again, probably not the best way to do it, but I did.
    char size_string[20];
    sprintf(size_string, "%d", size);
    fflush(stdout);
    strncat(arr[0], size_string, strlen(size_string));
    // returns the array of strings
    return(arr);
}