#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
    CS 361 HW2 instructions:
    - Display a command prompt and read in a command line from the user
    - Parse the command line into arguments, creating an array of character pointers, where array[0] points to the actual command and rest of the array elements point to the arguments to the command (Similar to main()’s argv[])
    - Fork off a child and have the child load the requested program by passing the argument vector created in step 2 to exec() family of system calls. The parent should report the PID of the child before proceeding to the next step.
    - Wait for the child to complete executing and report why it ended (exited or uncaught signal) and its exit value if available.
    - Repeat for first step forever till user enters the command exit
    - Your shell should also support basic I/O redirection line the unix shell.
    - $ command > filename Redirects the output of command to filename. The existing contents of filename are overwritten.
    - $ command >> filename` Redirects the output of command to filename. The output from command is appendend to contents of filename. Existing contents are not overwritten.
    - $ command < filename Command reads its input from filename instead of from stdin.
    - Your shell should handle the following signals:
    - SIGINT - Generated by Ctrl-C. This signal allows a user to teminate a running program. Your shell should not exit when user presses Ctrl-C but simply report that SIGINT signal has been received by the shell.
    - SIGTSTP - Generated by Ctrl-Z. Your shell should not exit when user presses Ctrl-Z but simply report that SIGTSTP signal has been received by the shell.
    - The shell need not support background processes or running more than one child at a time.
*/


// returns number of tokens for given char*. Uses space as delimiter.
int get_token_count(char * input) {
    int count = 0;
    char * temp;
    temp = strtok(input, " ");
    while (temp != NULL) {
        temp = strtok(NULL, " ");
        count++;
    }
    return count;
}


int main(int argc, char *argv[]) {

    printf("Welcome to bashsh, Basheer's shell... Enter your commands:\n");

    // while(1) {
    // display prompt
    printf("> ");


    // create a char pointer to hold in command and arguments
    char input[100];

    // read in line of input
    fgets(input, 100, stdin);
    char input_copy[100];
    strcpy(input_copy, input);

    // count number of tokens
    int num_tokens = get_token_count(input_copy);

    // create array of char pointers to store tokens
    char * current_token;
    char **tokens = (char **)malloc(sizeof(char *) * num_tokens);
    // parse the input and fill up the char pointers with the tokens
    current_token = strtok(input, " ");
    int i;
    for (i = 0; i < num_tokens; i++) {
        printf("%d: %s\n", i, current_token);  // DEBUG
        // create space for new token and copy it
        tokens[i] = (char *)malloc(sizeof(char) * strlen(current_token));
        strcpy(tokens[i], current_token);

        // get next token
        current_token = strtok(NULL, " ");
    }


    // DEBUG
    for (i = 0; i < num_tokens; i++) {
        printf("%d: %s\n", i, tokens[i]);
    }

    // DEBUG
    printf("number of tokens: %d\n", num_tokens);

    // fork a child and exec() command

    // free memory that held previous command
    for (i = 0; i < num_tokens; i++)
        free(tokens[i]);
    free(tokens);

    // report result, repeat above

    // }
    return 0;
}

// catch signals
