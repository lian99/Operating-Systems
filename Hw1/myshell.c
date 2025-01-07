#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> 

#define BUFFER_SIZE 100

/*This function duplicates a string by allocating new memoryand copying the content from the given string.It's used for storing commands in the history array.*/
char* my_strdup(const char* str) {
    int len = strlen(str) + 1; 
    char* new_str = malloc(len); // Allocate memory
    if (new_str == NULL) return NULL; // Return NULL if malloc failed
    memcpy(new_str, str, len); // Copy the string 
    return new_str;
}

int main(void) {
    close(2);
    dup(1);
    char command[BUFFER_SIZE]; /* buffer to store the user's input*/
    char* history[100]; // Store up to 100 commands
    int history_count = 0; /*counter to track how many commands have been stored*/

    while (1) {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);

        command[strcspn(command, "\n")] = 0; // Remove newline character from input

        /*If the command is "exit", the shell terminates*/
        if (strncmp(command, "exit", 4) == 0 && (strlen(command) == 4 || isspace(command[4]))) {
            break;
        }

        // Store command in history
        history[history_count] = my_strdup(command);
        history_count++;

        /*If the command is "history", it prints all stored commands in reverse order*/
        if (strncmp(command, "history", 7) == 0) {
            for (int i = 0; i < history_count; i++) {
                printf("%d %s\n", history_count - i, history[history_count - 1 - i]);
            }
            continue; // Continue to the next loop iteration
        }

        /*Checks if the command ends with & to determine if it should run in the background.If so, it sets the background flag and removes the &*/
        int background = 0;
        if (command[strlen(command) - 1] == '&') {
            background = 1;
            command[strlen(command) - 1] = '\0'; // Remove the '&' from command
        }

        //The command is split into arguments.
        //The shell forks a new process to execute the command.If it's a background process, it doesn't wait; otherwise, it waits for the command to finish.

        char* args[BUFFER_SIZE];
        int arg_count = 0;

     
        char* token = strtok(command, " ");
        while (token != NULL) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL; 

        // Fork a process to execute the command
        pid_t pid = fork();
        if (pid == 0) { // Child process
            execvp(args[0], args);
            perror("error"); // Exec only returns on error
            exit(1);  
        }
        else if (pid > 0) { // Parent process
            if (!background) {
                wait(NULL); // Wait for the child process to finish
            }
        }
        else {
            perror("error");
        }
    }

    // Clean up history
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }

    return 0;
}
