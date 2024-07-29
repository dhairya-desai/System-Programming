/*
ASSIGNMENT 2
NAME: DHAIRYA DESAI
ID: 110136228
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>

// Function to determine if a process is a descendant of a root process
int is_descendant(pid_t root_process, pid_t process_id) {
    char path[256]; 
    snprintf(path, sizeof(path), "/proc/%d/status", process_id); // Construct the path to the status file of the process_id

    FILE *file = fopen(path, "r");   // Open the status file to read the parent PID (PPid) of the process
    if (!file) return 0; // If the file cannot be opened, return 0 (not a descendant)

    pid_t ppid;
    while (fscanf(file, "PPid:\t%d", &ppid) != 1) {   // Read lines from the status file until the PPid line is found and extract PPid
        fscanf(file, "%*[^\n]\n"); // Skip lines until PPid is found
    }
    fclose(file); // Close the status file after reading PPid

    // If the parent PID is equal to the root_process, process_id is a direct descendant
    if (ppid == root_process) return 1; // Direct descendant
    // If the parent PID is 1, process_id has reached the init process, thus not a descendant
    if (ppid == 1) return 0; // Reached init process
    return is_descendant(root_process, ppid); // Recursively check if the parent process is a descendant of root_process 
}

// Function to list process IDs and their parent process IDs
void list_pids(pid_t root_process, pid_t process_id) {
    char path[256], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", process_id); // Construct the path to the status file of process_id

    FILE *file = fopen(path, "r"); // Open the status file to read the parent PID (PPid) of the process_id
    if (!file) {
        printf("The process %d does not belong to any process ", process_id);
        return;
    }

    pid_t ppid;
    while (fgets(line, sizeof(line), file)) { // Read each line of the status file
        if (sscanf(line, "PPid:\t%d", &ppid) == 1) { // Extract the parent PID (PPid) of the process
            break;
        }
    }
    fclose(file); // Close the status file after reading PPid

    if (is_descendant(root_process, process_id)) { // Check if the process_id is a descendant of root_process
        // If true, print the process_id and its parent process ID (PPid)
        printf("Process ID: %d, Parent Process ID: %d\n", process_id, ppid);
    } else {
        // If false, print a message indicating the process does not belong to the tree
        printf("The process %d does not belong to the tree rooted at %d\n", process_id, root_process);
    }
}

// Function to traverse the process tree and apply an action to all descendants
void traverse_and_act(pid_t root_process, void (*action)(pid_t)) {
    // Open the /proc directory to read process information.
    DIR *dir = opendir("/proc");
    
    struct dirent *entry;
    // Iterate over each entry in the /proc directory.
    while ((entry = readdir(dir)) != NULL) {
        // Check if the entry is a directory (which represents a process).
        if (entry->d_type == DT_DIR) {
            // Convert the directory name (which is a string) to a process ID (PID).
            pid_t pid = atoi(entry->d_name);
            // If the conversion is successful (PID > 0) and the process is a descendant of the root process,
            // apply the specified action to this process.
            if (pid > 0 && is_descendant(root_process, pid)) {
                action(pid);
            }
        }
    }
    closedir(dir); // Close the /proc directory after processing all entries.
}

// Functions to send specific signals to a process
void send_kill(pid_t pid) { // Sends the SIGKILL signal to the specified process
    kill(pid, SIGKILL);
}

void send_stop(pid_t pid) { // Sends the SIGSTOP signal to the specified process.
    kill(pid, SIGSTOP);
}

void send_continue(pid_t pid) { // Sends the SIGCONT signal to the specified process.
    kill(pid, SIGCONT);
}

// Functions to send signals to all descendants
void kill_descendants(pid_t root_process) { // Kills all descendant processes of the specified root process.
    traverse_and_act(root_process, send_kill);
}

void stop_descendants(pid_t root_process) { // Stops all descendant processes of the specified root process.
    traverse_and_act(root_process, send_stop);
}

void continue_descendants(pid_t root_process) { // Continues all paused descendant processes of the specified root process.
    traverse_and_act(root_process, send_continue);
}

// Function to list non-direct descendants
void list_non_direct_descendants(pid_t root_process, pid_t process_id) {
    DIR *dir = opendir("/proc"); // Open the /proc directory to read process information.
    
    struct dirent *entry;
    int found = 0; // Flag to indicate if any non-direct descendants were found
    
    while ((entry = readdir(dir)) != NULL) { // Iterate over each entry in the /proc directory.
        if (entry->d_type == DT_DIR) { // Check if the entry is a directory (which represents a process).
            pid_t pid = atoi(entry->d_name);  // Convert the directory name (which is a string) to a process ID (PID).
            
            // If the conversion is successful (PID > 0) and the process is a descendant
            // of the given process_id and not the process_id itself
            if (pid > 0 && is_descendant(process_id, pid) && pid != process_id) {
                char path[256], line[256]; // Construct the path to the status file of the process
                snprintf(path, sizeof(path), "/proc/%d/status", pid);
            
                FILE *file = fopen(path, "r");   // Open the status file to read process information
                if (!file) continue;

                pid_t ppid;
                while (fgets(line, sizeof(line), file)) {  // Read each line of the status file to find the parent PID (PPid)
                    if (sscanf(line, "PPid:\t%d", &ppid) == 1) {
                        break;
                    }
                }
                fclose(file);
                
                if (ppid != process_id) { // Check if the parent PID (PPid) is not the given process_id
                    printf("%d\n", pid);  // Print the PID of the non-direct descendant process
                    found = 1; // Set the flag indicating a non-direct descendant was found
                }
            }
        }
    }
    closedir(dir); // Close the /proc directory after processing all entries
    if (!found) { // If no non-direct descendants were found, print a message
        printf("No non-direct descendants\n");
    }
}

// Function to list immediate descendants
void list_immediate_descendants(pid_t root_process, pid_t process_id) {
    DIR *dir = opendir("/proc"); // Open the /proc directory to read process information
    
    struct dirent *entry;
    int found = 0; // Flag to indicate if any immediate descendants were found
    
    while ((entry = readdir(dir)) != NULL) {  // Iterate over each entry in the /proc directory
        if (entry->d_type == DT_DIR) { // Check if the entry is a directory (which represents a process)
            pid_t pid = atoi(entry->d_name); // Convert the directory name (which is a string) to a process ID (PID)
            
            // If the conversion is successful (PID > 0) and the process is a descendant
            // of the given process_id and not the process_id itself
            if (pid > 0 && is_descendant(process_id, pid) && pid != process_id) {
                char path[256], line[256];  // Construct the path to the status file of the process
                snprintf(path, sizeof(path), "/proc/%d/status", pid);
                
                FILE *file = fopen(path, "r");   // Open the status file to read process information
                if (!file) continue;

                pid_t ppid;
                while (fgets(line, sizeof(line), file)) { // Read each line of the status file to find the parent PID (PPid)
                    if (sscanf(line, "PPid:\t%d", &ppid) == 1) { // If the line contains the parent PID, extract it
                        break;
                    }
                }
                fclose(file);
                
                if (ppid == process_id) { // Check if the parent PID (PPid) is equal to the given process_id
                    printf("%d\n", pid); // Print the PID of the immediate descendant process
                    found = 1; // Set the flag indicating an immediate descendant was found
                }
            }
        }
    }
    closedir(dir); // Close the /proc directory after processing all entries.
    if (!found) {   // If no immediate descendants were found, print a message
        printf("No direct descendants\n");
    }
}

// Function to list sibling processes
void list_sibling_processes(pid_t root_process, pid_t process_id) {
    char path[256], line[256]; // Construct the path to the status file of the given process
    snprintf(path, sizeof(path), "/proc/%d/status", process_id);

    FILE *file = fopen(path, "r");  // Open the status file to read the parent PID (PPid) of the given process
    if (!file) {
        printf("Failed to open status file for process %d\n", process_id);
        return;
    }

    pid_t ppid;
    while (fgets(line, sizeof(line), file)) { // Read each line of the status file
        if (sscanf(line, "PPid:\t%d", &ppid) == 1) { // Extract the parent PID (PPid) of the process
            break;
        }
    }
    fclose(file); // Close the status file after reading PPid

    DIR *dir = opendir("/proc");  // Open the /proc directory to read process information
    if (!dir) {
        printf("Failed to open /proc directory\n");
        return;
    }

    struct dirent *entry;
    int found = 0; // Flag to indicate if any siblings were found

    while ((entry = readdir(dir)) != NULL) { // Iterate over each entry in the /proc directory
        if (entry->d_type == DT_DIR) {  // Check if the entry is a directory (which represents a process)
            pid_t pid = atoi(entry->d_name); // Convert the directory name (which is a string) to a process ID (PID)
            if (pid > 0 && pid != process_id) { // If the PID is valid and not the same as the given process ID
                snprintf(path, sizeof(path), "/proc/%d/status", pid); // Construct the path to the status file of the sibling process

                file = fopen(path, "r"); // Open the status file to read process information
                if (!file) continue;

                pid_t sibling_ppid;
                while (fgets(line, sizeof(line), file)) { // Read each line of the status file
                    if (sscanf(line, "PPid:\t%d", &sibling_ppid) == 1) { // Extract the parent PID (PPid) of the sibling process
                        break;
                    }
                }
                fclose(file); // Close the status file after reading PPid

                if (sibling_ppid == ppid) { // Check if the parent PID of the sibling process matches the parent PID of the given process
                    printf("%d\n", pid);
                    found = 1; // Set the flag indicating a sibling was found
                }
            }
        }
    }
    closedir(dir); // Close the /proc directory after processing all entries
    if (!found) { // If no siblings were found, print a message
        printf("No siblings\n");
    }
}

// Function to list defunct sibling processes
void list_defunct_siblings(pid_t root_process, pid_t process_id) {
    char path[256], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", process_id); // Construct the path to the status file of the given process

    FILE *file = fopen(path, "r"); // Open the status file to read the parent PID (PPid) of the given process
    if (!file) {
        perror("Failed to open status file");
        return;
    }

    pid_t ppid;
    while (fgets(line, sizeof(line), file)) { // Read each line of the status file
        if (sscanf(line, "PPid:\t%d", &ppid) == 1) { // Extract the parent PID (PPid) of the process
            break;
        }
    }
    fclose(file); // Close the status file after reading PPid

    DIR *dir = opendir("/proc"); // Open the /proc directory to read process information
    if (!dir) {
        perror("Failed to open /proc directory");
        return;
    }

    struct dirent *entry;
    int found = 0; // Flag to indicate if any defunct siblings were found

    while ((entry = readdir(dir)) != NULL) { // Iterate over each entry in the /proc directory
        if (entry->d_type == DT_DIR) { // Check if the entry is a directory (which represents a process)
            pid_t pid = atoi(entry->d_name); // Convert the directory name (which is a string) to a process ID (PID)
            if (pid > 0 && pid != process_id) { // If the PID is valid and not the same as the given process ID
                snprintf(path, sizeof(path), "/proc/%d/status", pid); // Construct the path to the status file of the sibling process
                file = fopen(path, "r"); // Open the status file to read process information
                if (!file) continue;

                int is_defunct = 0;
                pid_t sibling_ppid = 0;
                while (fgets(line, sizeof(line), file)) { // Read each line of the status file
                    if (strncmp(line, "State:\t", 7) == 0 && strstr(line, "Z (zombie)") != NULL) { // Check if the process is in a defunct (zombie) state
                        is_defunct = 1;
                    }
                    if (sscanf(line, "PPid:\t%d", &sibling_ppid) == 1) { // Extract the parent PID (PPid) of the sibling process
                        break;
                    }
                }
                fclose(file); // Close the status file after reading PPid

                // Check if the parent PID matches the parent PID of the given process
                // and if the sibling process is defunct
                if (sibling_ppid == ppid && is_defunct) {
                    printf("%d\n", pid); // Print the PID of the defunct sibling process
                    found = 1; // Set the flag indicating a defunct sibling was found
                }
            }
        }
    }
    closedir(dir); // Close the /proc directory after processing all entries
    if (!found) { // If no defunct siblings were found, print a message
        printf("No defunct siblings\n");
    }
}

// Function to list defunct descendants
void list_defunct_descendants(pid_t root_process, pid_t process_id) {
    DIR *dir = opendir("/proc"); // Open the /proc directory
    if (!dir) {
        perror("Failed to open /proc directory");
        return;
    }

    struct dirent *entry;
    int found = 0; // Flag to indicate if any defunct descendants were found
    while ((entry = readdir(dir)) != NULL) { // Iterate over each entry in /proc
        if (entry->d_type == DT_DIR) { // Check if the entry is a directory
            pid_t pid = atoi(entry->d_name); // Convert directory name to PID
            if (pid > 0 && is_descendant(process_id, pid) && pid != process_id) { // Check if PID is a descendant
                char path[256], line[256];
                snprintf(path, sizeof(path), "/proc/%d/status", pid); // Construct path to status file
                FILE *file = fopen(path, "r");
                if (!file) continue;

                while (fgets(line, sizeof(line), file)) { // Read each line of the status file to find the process state
                    if (strncmp(line, "State:\t", 7) == 0 && strstr(line, "Z (zombie)") != NULL) { // Check if the process is in a defunct (zombie) state
                        printf("%d\n", pid); // Print PID if defunct
                        found = 1; // Set the flag indicating a defunct descendant was found
                        break;
                    }
                }
                fclose(file); // Close the status file
            }
        }
    }
    closedir(dir); // Close the /proc directory

    if (!found) {
        printf("No defunct descendants\n");
    }
}

// Function to list grandchildren processes
void list_grandchildren(pid_t root_process, pid_t process_id) {
    DIR *dir = opendir("/proc"); // Open the /proc directory
    struct dirent *entry;
    int found = 0; // Flag to indicate if any grandchildren were found
    while ((entry = readdir(dir)) != NULL) { // Iterate over each entry in /proc
        if (entry->d_type == DT_DIR) { // Check if the entry is a directory
            pid_t pid = atoi(entry->d_name); // Convert directory name to PID
            if (pid > 0 && is_descendant(process_id, pid) && pid != process_id) { // Check if PID is a descendant
                char path[256], line[256];
                snprintf(path, sizeof(path), "/proc/%d/status", pid); // Construct path to status file
                FILE *file = fopen(path, "r");
                if (!file) continue;

                pid_t ppid;
                
                while (fscanf(file, "PPid:\t%d", &ppid) != 1) {// Read the parent PID (PPid) from the status file
                    fscanf(file, "%*[^\n]\n"); // Skip lines until PPid is found
                }
                fclose(file); // Close the status file

                if (is_descendant(process_id, ppid) && ppid != process_id) {   // Check if the parent PID is a descendant of the given process_id
                    printf("%d\n", pid); // Print PID if it is a grandchild
                    found = 1; // Set the flag indicating a grandchild was found
                }
            }
        }
    }
    closedir(dir); // Close the /proc directory

    if (!found) {
        printf("No grandchildren\n");
    }
}

// Function to print if a process is defunct (zombie) or not
void print_status_defunct(pid_t root_process, pid_t process_id) {
    char path[256], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", process_id); // Construct path to status file

    // Open the status file to read the state of the process
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Failed to open status file");
        return;
    }

    while (fgets(line, sizeof(line), file)) { // Read each line of the status file
        if (strncmp(line, "State:\t", 7) == 0) { // Check if the line starts with "State:"
            if (strstr(line, "Z (zombie)") != NULL) { // Check if the process is in a defunct (zombie) state
                printf("Defunct\n"); // Print "Defunct" if the process is a zombie
                fclose(file); // Close the status file
                return;
            } else {
                printf("Not defunct\n"); // Print "Not defunct" if the process is not a zombie
                fclose(file); // Close the status file
                return;
            }
        }
    }

    fclose(file); // Close the status file
    printf("Not defunct\n"); // Default to "Not defunct" if the state line is not found
}

// Function to kill parents of all zombie processes
void kill_parents_of_zombies(pid_t root_process, pid_t process_id) {
    DIR *dir = opendir("/proc"); // Open the /proc directory
    if (!dir) {
        perror("Failed to open /proc directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) { // Iterate over each entry in /proc
        if (entry->d_type == DT_DIR) { // Check if the entry is a directory
            pid_t pid = atoi(entry->d_name); // Convert directory name to PID
            if (pid > 0 && is_descendant(process_id, pid) && pid != process_id) { // Check if PID is a descendant
                char path[256], line[256];
                snprintf(path, sizeof(path), "/proc/%d/status", pid); // Construct path to status file
                FILE *file = fopen(path, "r");
                if (!file) continue;

                int is_zombie = 0;
                pid_t ppid = 0;
                while (fgets(line, sizeof(line), file)) { // Read each line of the status file
                    if (strncmp(line, "State:\t", 7) == 0 && strstr(line, "Z (zombie)") != NULL) { // Check if the process is in a defunct (zombie) state
                        is_zombie = 1;
                    }
                    if (sscanf(line, "PPid:\t%d", &ppid) == 1) { // Extract the parent PID (PPid) of the process
                        break;
                    }
                }
                fclose(file); // Close the status file

                if (is_zombie && ppid > 0) { // If the process is a zombie and has a valid parent PID
                    printf("Killing parent process %d of zombie process %d\n", ppid, pid);
                    kill(ppid, SIGKILL); // Kill the parent process
                }
            }
        }
    }
    closedir(dir); // Close the /proc directory
}


// Function to handle different command options
void handle_option(const char *option, pid_t root_process, pid_t process_id) {
    // Determine the action to take based on the option provided
    switch (option[1]) { // Use the second character of the option string for the switch-case
        case 'd':
            switch (option[2]) {
                case 'x':
                    kill_descendants(root_process); // Kill all descendant processes of the root process
                    break;
                case 't':
                    stop_descendants(root_process); // Stop all descendant processes of the root process
                    break;
                case 'c':
                    continue_descendants(root_process);  // Continue all paused descendant processes of the root process
                    break;
                case 'd':
                    list_immediate_descendants(root_process, process_id); // List immediate descendants of the given process_id
                    break;
                default:
                    printf("Invalid option\n");
                    break;
            }
            break;
        case 'r':
            if (strcmp(option, "-rp") == 0) {
                if (is_descendant(root_process, process_id)) {  // Kill the given process if it is a descendant of the root process
                    kill(process_id, SIGKILL);
                } else {
                    printf("The process %d does not belong to the tree rooted at %d\n", process_id, root_process);
                }
            } else {
                printf("Invalid option\n");
            }
            break;
        case 'n':
            if (strcmp(option, "-nd") == 0) {
                list_non_direct_descendants(root_process, process_id);  // List non-direct descendants of the given process_id
            } else {
                printf("Invalid option\n");
            }
            break;
        case 's':
            if (strcmp(option, "-sb") == 0) {
                list_sibling_processes(root_process, process_id); // List sibling processes of the given process_id
            } else if (strcmp(option, "-sz") == 0) {
                print_status_defunct(root_process, process_id); // Print if the given process_id is defunct (zombie) or not
            } else { 
                printf("Invalid option\n");
            }
            break;
        case 'b':
            if (strcmp(option, "-bz") == 0) {
                list_defunct_siblings(root_process, process_id); // List defunct sibling processes of the given process_id
            } else {
                printf("Invalid option\n");
            }
            break;
        case 'z':
            if (strcmp(option, "-zd") == 0) {
                list_defunct_descendants(root_process, process_id);// List defunct descendants of the given process_id
            } else {
                printf("Invalid option\n");
            }
            break;
        case 'g':
            if (strcmp(option, "-gc") == 0) {
                list_grandchildren(root_process, process_id); // List grandchildren of the given process_id
            } else {
                printf("Invalid option\n");
            }
            break;
        case 'k':
            if (strcmp(option, "-kz") == 0) {
                kill_parents_of_zombies(root_process, process_id);  // Kill parents of all zombie processes descended from the given process_id
            } else {
                printf("Invalid option\n");
            }
            break;
        default:
            printf("Invalid option\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) { // Check if the number of arguments is valid
        fprintf(stderr, "Usage: %s [Option] [root_process] [process_id]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t root_process = atoi(argv[argc - 2]); // Convert the second last argument to the root process ID
    pid_t process_id = atoi(argv[argc - 1]);  // Convert the last argument to the process ID

    // Using ternary operator to call the appropriate function based on the number of arguments
    (argc == 3) ? list_pids(root_process, process_id) : handle_option(argv[1], root_process, process_id);

    return 0; 
}
