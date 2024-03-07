#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define LOG_FILE "shell.log"
#define MAX_COMMAND_LENGTH  100
#define MAX_ARGUMENTS       64

#define ORANGE          "\033[0;33m"
#define GREEN           "\033[0;32m"


// Function prototypes
void parent_main();
void setup_environment();
void on_child_exit();
void shell();
void parse_input(char *input);
void execute_command(char *command, char *args[], int background);
void execute_shell_builtin(char *command, char *args[], int background);
void reap_child_zombie();
void evaluate_expression(char *input);

FILE *log_file;

int main(){
    parent_main();
    return 0;
}


void parent_main(){
    // Register signal handler for SIGCHLD
    signal(SIGCHLD, reap_child_zombie);

    // Open log file
    
    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    // Setup environment
    setup_environment();

    // Start shell
    shell();

    // Close log file
    fclose(log_file);

}

void setup_environment() {
    // Set current working directory
    char arr[100];
    chdir(getcwd(arr, 100)); 
}

void shell() {
    char input[MAX_COMMAND_LENGTH];
    int command_is_not_exit = 1;

    do {
        char cwd[100];
        getcwd(cwd, sizeof(cwd));
        printf(ORANGE);
        printf("vboxuser@Hosam:~");
        printf(GREEN);
        printf("%*s > ", (int)strlen(cwd) -  14, cwd + 14);
        fgets(input, MAX_COMMAND_LENGTH, stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove newline character

        evaluate_expression(input);
        parse_input(input);
        

        if (strcmp(input, "exit") == 0) {
            command_is_not_exit = 0;
        }
    } while (command_is_not_exit);


}

void parse_input(char *input) {
    char *token;
    char *args[MAX_ARGUMENTS];
    int arg_count = 0;
    int background = 0; // Flag to indicate background execution

    // Tokenize input string by whitespace
    token = strtok(input, " ");
    while (token != NULL && arg_count < MAX_ARGUMENTS - 1) {
        if (strcmp(token, "&") == 0) {
            background = 1; // Set background flag if '&' is encountered
        } else {
            args[arg_count] = token; // Store token in argument array
            arg_count++;
        }
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; // Null-terminate the argument array

    // Execute command
    if (arg_count > 0) {
        if(!strcmp(args[0],"cd") || !strcmp(args[0],"echo") || !strcmp(args[0],"export") ){
            execute_shell_builtin(args[0], args, background);
        }else{
            execute_command(args[0], args, background);
        }
        
    }
}


void execute_command(char *command, char *args[], int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
    } else if (pid == 0) { // Child process
        execvp(command, args);
        // If execvp returns, it must have failed
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else { // Parent process
        if (!background) {
            waitpid(pid, NULL, 0); // Wait for child process if not in background
        }
    }
}

void execute_shell_builtin(char *command, char *args[], int background) {
    if (strcmp(command, "cd") == 0) {
        // Handle cd command
        char *path = args[1]; // Extract path from arguments
        if (path == NULL) {
            // Change to home directory if no path provided
            path = getenv("HOME");
        }
        if (chdir(path) != 0) {
            perror("cd");
        }
    } else if (strcmp(command, "echo") == 0) {
        // Handle echo command
        char *message = args[1]; // Extract message from arguments
        printf("%s\n", message); // Print the message
    } else if (strcmp(command, "export") == 0) 
    {
        char *token = strtok(args[1], "="); // Tokenize the argument by "="
        char *parsedInput[100];
        int counter = 0;
        while (token != NULL && counter < 2) {
            parsedInput[counter] = token;
            token = strtok(NULL, "=");
            counter++;
        }
        parsedInput[counter] = NULL;

        if (parsedInput[1] != NULL) {
            char *data = parsedInput[1];
            if (data[0] == '"') {
                data++;
                data[strlen(data) - 1] = '\0';
                setenv(parsedInput[0], data, 1);
            } else {
                setenv(parsedInput[0], parsedInput[1], 1);
            }
        } else {
            printf("Invalid export command format\n");
        }

    } else {
        printf("Unknown shell built-in command\n");
    }
}

void evaluate_expression(char *input) {
    int i;
    for (i = 0; input[i] != '\0'; i++) {
        if (input[i] == '$' && input[i + 1] != '\0' && input[i + 1] != ' ') {
            // Extract the variable name
            int j = i + 1;
            while (input[j] != '\0' && input[j] != ' ') {
                j++;
            }
            char variable_name[j - i];
            strncpy(variable_name, &input[i + 1], j - i - 1);
            variable_name[j - i - 1] = '\0';
            
            // Replace the variable name with its value
            char *value = getenv(variable_name);
            if (value != NULL) {
                // Calculate length difference for resizing
                int value_len = strlen(value);
                int variable_len = j - i - 1;
                int length_diff = value_len - variable_len;
                
                // Resize the input string if necessary
                if (length_diff != 0) {
                    memmove(&input[i + value_len], &input[j], strlen(&input[j]) + 1);
                }
                
                // Replace the variable name with its value
                memmove(&input[i], value, value_len);
                i += value_len - 1; // Move the index to the end of the replaced value
            }
        }
    }
}

void reap_child_zombie() {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        fprintf(log_file, "Child process was terminated\n");
    }
}