/*
 DHAIRYA DESAI
 110136228- ASSIGNMENT 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 5
#define MAX_BG_PROCESSES 10

pid_t bg_processes[MAX_BG_PROCESSES]; // Array to keep track of background processes
int bg_count = 0;

// Function Prototypes
void process_command(char *input, char **args, int *argc);
int execute_command(char **args, int argc);
void handle_special_commands(char *input);
void handle_redirection(char *input);
void execute_command_with_redirect(char **args, char *infile, char *outfile, int append);
void execute_pipeline(char **commands, int count);
void count_words_in_file(char *filename);
void concatenate_files(char *input);
void run_in_background(char **args, int argc);
void bring_to_foreground();
void execute_sequential_commands(char *input);
void execute_conditional_commands(char *input);
int contains_special_character(char *input);
void split_commands(char *input, char **commands, char *delimiter, int *count);

//Main Function
int main()
{
    char input[MAX_INPUT_SIZE]; // Buffer to store user input
    char *args[MAX_ARG_COUNT + 1]; // Array to store command arguments
    int argc; // Argument count

    while (1)
    {
        printf("minibash$ "); // Display prompt
        if (fgets(input, sizeof(input), stdin) == NULL)  // Read input
        {
            perror("fgets failed");  // Error handling for input failure
            continue;
        }

        input[strcspn(input, "\n")] = 0; // Remove newline character from input

        if (strcmp(input, "dter") == 0) // Check for 'dter' command to exit shell
        {
            printf("Exiting minibash\n");
            exit(0);
        }

        if (strcmp(input, "fore") == 0) // Check for 'fore' command to bring background process to foreground
        {
            bring_to_foreground();
            continue;
        }

        if (contains_special_character(input)) // Check for special characters in input
        {
            handle_special_commands(input);
        }
        else
        {
            process_command(input, args, &argc);
            if (argc > 0 && argc <= 4)
            {
                execute_command(args, argc);
            }
            else
            {
                printf("Invalid Arguments Error. Arguments must be greater than or equa1 or less than or equal to 4.\n"); // Handle invalid argument count
            }
        }
    }
    return 0;
}

// Splits the input command into arguments and counts them
void process_command(char *input, char **args, int *argc)
{
    int i = 0;
    args[i] = strtok(input, " "); // Split input by space
    while (args[i] != NULL && i < MAX_ARG_COUNT + 1)
    {
        i++;
        args[i] = strtok(NULL, " ");  // Continue splitting by spaces
    }
    *argc = i; // Store argument count
}

// Forks and executes a command
int execute_command(char **args, int argc)
{
    pid_t pid, wpid;
    int status;

    pid = fork(); // Create a child process
    if (pid == -1)
    {
        perror("fork failed"); // Handle fork error
        return -1;
    }
    else if (pid == 0)
    {
        if (execvp(args[0], args) == -1) // Execute command in child process
        {
            perror("exec failed"); // Handle exec error
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED); // Wait for child process to finish
            if (wpid == -1)
            {
                perror("waitpid failed");
                return -1;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));  // Check if process has exited or was terminated
    } 

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1; // Return exit status
}

// Handles special commands with special characters
void handle_special_commands(char *input)
{
    if (input[0] == '#') 
    {
        count_words_in_file(input + 1); // Handle word count command 
    }
    else if (strchr(input, '~'))
    {
        concatenate_files(input); // Handle file concatenation command
    }
    else if (strchr(input, '>') || strchr(input, '<'))
    {
        handle_redirection(input); // Handle I/O redirection command
    }
    else if (strstr(input, "&&") || strstr(input, "||"))
    {
        execute_conditional_commands(input); // Handle conditional commands
    }
    else if (strchr(input, '|'))
    {
        char *commands[4];
        int count = 0;
        split_commands(input, commands, "|", &count);  // Split pipeline commands
        execute_pipeline(commands, count); // Execute pipeline commands
    }
    else if (strchr(input, ';'))
    {
        execute_sequential_commands(input); // Handle sequential commands
    }
    else
    {
        char *args[MAX_ARG_COUNT + 1];
        int argc;
        process_command(input, args, &argc); // Process normal commands
        if (argc > 0 && argc <= MAX_ARG_COUNT)
        {
            if (args[argc - 1] && strcmp(args[argc - 1], "+") == 0)
            {
                args[argc - 1] = NULL;
                run_in_background(args, argc - 1);  // Handle background command
            }
            else
            {
                execute_command(args, argc);  // Execute normal commands
            }
        }
        else
        {
            printf("Invalid Arguments Error\n"); // Handle invalid argument count
        }
    }
}

// Handles input and output redirection
void handle_redirection(char *input)
{
    char *args[MAX_ARG_COUNT + 1]; // Array to hold command arguments
    int argc = 0; // Counter to keep track of the number of arguments
    char *token = strtok(input, " "); // Tokenize the input string by spaces

    while (token != NULL && argc < MAX_ARG_COUNT + 1)
    {
        args[argc++] = token; // Store each token (argument) in the args array
        token = strtok(NULL, " "); // Continue tokenizing the input string
    }
    args[argc] = NULL;  // Null-terminate the args array

    if (argc > MAX_ARG_COUNT)
    {
        // Check if the number of arguments exceeds the maximum allowed
        printf("Invalid Arguments Error\n");
        return;
    }

    int in = 0, out = 0, append = 0; // Flags to indicate input/output redirection and append mode
    char *infile = NULL, *outfile = NULL; // Pointers to hold input and output file names

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            // If input redirection is specified
            in = 1; // Set the input redirection flag
            infile = args[i + 1]; // Get the input file name
            args[i] = NULL; // Nullify the redirection operator in args array
        }
        else if (strcmp(args[i], ">") == 0)
        {
            // If output redirection is specified
            out = 1; // Set the output redirection flag
            outfile = args[i + 1]; // Get the output file name
            args[i] = NULL; // Nullify the redirection operator in args array
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            // If output append redirection is specified
            append = 1; // Set the append flag
            outfile = args[i + 1]; // Get the output file name
            args[i] = NULL; // Nullify the redirection operator in args array
        }
    }

    execute_command_with_redirect(args, infile, outfile, append);
}

// Function to execute a command with input/output redirection
void execute_command_with_redirect(char **args, char *infile, char *outfile, int append)
{
   pid_t pid, wpid; // Variables to store process IDs
    int status; // Variable to store the status of the child process
    int fd; // File descriptor for file operations

    pid = fork(); // Create a new process
    if (pid == -1)
    {
        perror("fork failed"); // Handle fork error
        return;
    }
    else if (pid == 0)
    {
        if (infile)
        {
            // If input redirection is specified
            fd = open(infile, O_RDONLY);
            if (fd == -1)
            {
                perror("open infile failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO); //  Redirect standard input to the input file
            close(fd); // Close the file descriptor
        }

        if (outfile)
        {
             // If output redirection is specified
            fd = open(outfile, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
            if (fd == -1)
            {
                perror("open outfile failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Redirect standard output to the output file
            close(fd);
        }

        if (execvp(args[0], args) == -1)
        {
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED); // Wait for the child process to change state
            if (wpid == -1)
            {
                perror("waitpid failed");
                return;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));   // Continue waiting until the child process exits or is killed
    }
}

//Function to execute conditional commands
void execute_conditional_commands(char *input)
{
 char *or_commands[4]; // Array to hold OR-separated commands
    int or_count = 0; // Counter to keep track of the number of OR-separated commands

    split_commands(input, or_commands, "||", &or_count); // Split the input into OR-separated commands

    // Loop through each OR-separated command
    for (int i = 0; i < or_count; i++)
    {
        char *and_commands[4];
        int and_count = 0;

        split_commands(or_commands[i], and_commands, "&&", &and_count);

        int execute_next_command = 1; // Flag to determine whether to execute the next command

        for (int j = 0; j < and_count; j++)
        {
            if (!execute_next_command)
                break;

            char *args[MAX_ARG_COUNT + 1];
            int argc;
            process_command(and_commands[j], args, &argc);

            if (argc > 0 && argc <= MAX_ARG_COUNT)  // If the number of arguments is valid, execute the command
            {
                int status = execute_command(args, argc); // Execute the command
                execute_next_command = (status == 0);  // Set the flag based on the command's success
            }
            else
            {
                printf("Invalid Arguments Error\n");  // If the number of arguments is invalid, print an error message and return
                return;
            }
        }

        if (execute_next_command)
            break; // If an AND-separated command sequence was successful, stop executing the remaining OR-separated commands
    }
    }

// Execute a pipeline of commands
void execute_pipeline(char **commands, int count)
{

    int i, in_fd = 0, fd[2]; // Initialize loop counter, input file descriptor, and file descriptor array for pipe
    pid_t pid;// Variable to hold process ID

    // Loop through each command in the pipeline
    for (i = 0; i < count; i++)
    {
        if (i < count - 1)
            pipe(fd); // Create a pipe for all but the last command
        pid = fork(); // Create a new process
        if (pid == -1)
        {
            perror("fork failed"); // Handle fork error
            return;
        }
        else if (pid == 0)
        {
            if (in_fd != 0)
            {
                dup2(in_fd, 0); // If not the first command, duplicate in_fd to standard input
                close(in_fd);   // Close the old in_fd
            }
            if (i < count - 1)
            {
                dup2(fd[1], 1); // If not the last command, duplicate fd[1] to standard output
                close(fd[1]);   // Close the write end of the pipe
            }

            // Parse command and count arguments
            char *args[4];
            int argc = 0;

            // Check argument count
            if (argc > 4)
            {
                fprintf(stderr, "Error: Too many arguments in command %d\n", i + 1);
                exit(EXIT_FAILURE);
            }

            process_command(commands[i], args, &argc); // Process the command to extract arguments

            // Execute command
            if (execvp(args[0], args) == -1)
            {
                perror("exec failed"); // Handle exec failure
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            wait(NULL); // Wait for the child process to finish
            if (in_fd != 0)
                close(in_fd); // Close the old input file descriptor
            if (i < count - 1)
            {
                close(fd[1]);  // Close the write end of the pipe
                in_fd = fd[0]; // Save the read end of the pipe for the next command
            }
        }
    }
}

// Count words in a file
void count_words_in_file(char *filename)
{
    FILE *file = fopen(filename, "r"); // Open the file for reading
    if (!file)
    {
        perror("fopen failed"); // If the file cannot be opened, print an error message and return
        return;
    }

    int word_count = 0;        // Initialize the word count to 0
    char word[MAX_INPUT_SIZE]; // Buffer to hold each word read from the file
    while (fscanf(file, "%s", word) != EOF)
        word_count++; // Increment the word count for each word read from the file

    fclose(file); // Close the file
    printf("Number of words in %s: %d\n", filename, word_count);
}

// Concatenate multiple files
void concatenate_files(char *input)
{
    char *token;
    char *args[4]; // Array to hold up to 4 filenames
    int argc = 0;  // Counter to keep track of the number of filenames

    // Tokenize the input string using ' ' and '~' as delimiters
    token = strtok(input, " ~");
    while (token != NULL)
    {

        args[argc++] = token;       // Store each token (filename) in the args array
        token = strtok(NULL, " ~"); // Get the next token

        // Check if the number of filenames exceeds the maximum allowed
        if (argc > 4)
        {
            printf("Invalid Arguments Error\n");
            return;
        }
    }

    char buffer[MAX_INPUT_SIZE]; // Buffer to hold lines read from the files

    // Loop through each filename
    for (int i = 0; i < argc; i++)
    {
        FILE *file = fopen(args[i], "r"); // Open the file for reading
        if (!file)
        {
            perror("fopen failed"); // If the file cannot be opened, print an error message and return
            return;
        }
        // Read and print lines from the file
        while (fgets(buffer, sizeof(buffer), file))
        {
            printf("%s", buffer); // Print each line to the standard output
            printf("\n");
        }
        fclose(file); // Close the file
    }
  
}

// Run a command in the background
void run_in_background(char **args, int argc)
{
    if (bg_count + argc > MAX_BG_PROCESSES) // Check if adding the new background processes will exceed the maximum allowed
    {
        printf("Too many background processes\n");
        return;
    }

    // Loop through each argument to create a new background process for each command
    for (int i = 0; i < argc; i++)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork failed");
            return;
        }
        else if (pid == 0)
        {
            int dev_null = open("/dev/null", O_WRONLY);
            if (dev_null == -1)
            {
                perror("open /dev/null failed");
                exit(EXIT_FAILURE);
            }
            dup2(dev_null, STDERR_FILENO); // Redirect standard error to /dev/null
            close(dev_null);               // Close the file descriptor for /dev/null

            char *single_arg[] = {args[i], NULL}; // Prepare to execute the command
            execvp(single_arg[0], single_arg);
            // If execvp fails, exit silently
            exit(EXIT_FAILURE);
        }
        else
        {
            bg_processes[bg_count++] = pid; // Store the PID of the background process
            printf("Running in background: %d\n", pid);
        }
    }
}

// Bring the last background process to the foreground
void bring_to_foreground()
{
    if (bg_count == 0) // Check if there are no background processes
    {
        printf("No background processes\n");
        return;
    }

    pid_t pid = bg_processes[--bg_count]; // Get the last background process
    int status;
    waitpid(pid, &status, 0);                         // Wait for the background process to complete
    printf("Foreground process %d completed\n", pid); // Inform the user that the process has completed
}

// Execute sequential commands separated by ';'
void execute_sequential_commands(char *input)
{
    char *commands[4]; // Array to hold up to 4 individual commands
    int count = 0;     // Counter to keep track of the number of commands

    split_commands(input, commands, ";", &count); // Split the input string into individual commands using ';' as the delimiter

    // Loop through each command
    for (int i = 0; i < count; i++)
    {
        char *args[4]; // Array to hold the arguments of a command
        int argc = 0;  // Counter to keep track of the number of arguments

        process_command(commands[i], args, &argc); // Process the current command to extract its arguments

        if (argc > 0 && argc <= 4)
        {
            execute_command(args, argc); // Execute the command if the number of arguments is valid
        }
        else
        {
            printf("Invalid Arguments Error\n"); // Print an error message if the number of arguments is invalid
        }
    }
}

// Check if the input string contains any of the specified special characters
int contains_special_character(char *input)
{
    return strchr(input, '#') || strchr(input, '~') || strchr(input, '|') ||
           strchr(input, '<') || strchr(input, '>') || strchr(input, ';') ||
           strchr(input, '&') || strchr(input, '+');
}

// Split input into commands based on delimiter
void split_commands(char *input, char **commands, char *delimiter, int *count)
{
    // Use strtok to split the input string into tokens based on the given delimite
    char *token = strtok(input, delimiter);
    while (token != NULL && *count < 4)
    {
        commands[(*count)++] = token;    // Store each token (command) in the commands array and increment the count
        token = strtok(NULL, delimiter); // Get the next token
    }
}
