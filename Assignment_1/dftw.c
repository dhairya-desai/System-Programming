/*
ASSIGNMENT 1
NAME: DHAIRYA DESAI
ID: 110136228
*/

#define PATH_MAX 4096
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

// Declare the Global variables to keep track of the number of files, directories, and to store the source and destination directory paths.
int file_count = 0, directory_count = 0; // Variables to store count of files and directories
char source_directory[PATH_MAX];
char destination_directory[PATH_MAX];
char *extension = NULL; // Variable to store the file extension to be excluded, if any

// Function to count the number of files and directories
int count_items(const char *filepath, const struct stat *info, int flag_type, struct FTW *pathinfo){
    (flag_type == FTW_F) ? file_count++ : (flag_type == FTW_D ? directory_count++ : 0); // Increase the count of files and directories
    return 0;
}

// Function to display the sizes of all the files
int size_of_files(const char *filepath, const struct stat *info, int flag_type, struct FTW *pathinfo){
    if (flag_type == FTW_F){
        printf("File: %s, Size: %ld bytes\n", strrchr(filepath, '/') ? strrchr(filepath, '/') + 1 : filepath, info->st_size); // Display the size of file along with its name
    }
    return 0;
}

// Function to copy files from source path to destination path
int copyFile(const char *sourcepath, const char *destinationpath){
    int source_file_descriptor, destination_file_descriptor; // Declare source and destination file descriptors
    char copyBuffer[4096];
    ssize_t bytesRead, bytesWrite;

    source_file_descriptor = open(sourcepath, O_RDONLY); // Open the source file in read-only mode

    // Error Check
    if (source_file_descriptor == -1){
        return -1;
    }

    // Open the destination file and create if it doesn't exist
    destination_file_descriptor = open(destinationpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination_file_descriptor == -1){
        close(source_file_descriptor); // Close the file descriptor
        return -1;
    }

    do{
        bytesRead = read(source_file_descriptor, copyBuffer, sizeof(copyBuffer)); // Read from the source file and write to the destination file
        if (bytesRead > 0){
            bytesWrite = write(destination_file_descriptor, copyBuffer, bytesRead);
            if (bytesWrite != bytesRead){
                close(source_file_descriptor);      // Close source file descriptor
                close(destination_file_descriptor); // Close destination file descriptor
                return -1;
            }
        }
    } while (bytesRead > 0); // Continue until there are no more bytes to read

    // Check for read errors
    int status = (bytesRead < 0) ? (perror("Error while reading from the source file"), -1) : 0;

    // Close both file descriptors
    close(source_file_descriptor);
    close(destination_file_descriptor);

    return status; // Return the status indicating success or failure
}

// Function to copy the directory hierarchy
int copy_directory_path(const char *filepath, const struct stat *storage_buffer, int flagtype, struct FTW *ftwbuf){
    char destination_path[PATH_MAX]; // Variable to store destination path

    // Construct the destination path by appending the relative path from the source directory to the destination directory
    snprintf(destination_path, sizeof(destination_path), "%s/%s", destination_directory, filepath + strlen(source_directory) + 1);

    if (flagtype == FTW_D){ // Check if the current item is a directory
        // Create the directory in the destination
        if (mkdir(destination_path, storage_buffer->st_mode) < 0 && errno != EEXIST){
            return -1; // If directory creation fails, return an error
        }
    }
    else if (flagtype == FTW_F){ // Check if the current item is a file
        // Copy the file to the destination if it doesn't match the excluded extension
        if (!extension || !strstr(filepath, extension)){
            // If there is no extension to exclude or the file's extension doesn't match the excluded extension
            if (copyFile(filepath, destination_path) < 0){
                return -1; // If the file copy operation fails, return an error
            }
        }
    }
    return 0; // Return 0 to indicate success
}

// Function to move files and directories from source path to destination path
int move_file(const char *filepath, const struct stat *sb, int flag_type, struct FTW *ftwbuf)
{
    char new_path[4096];
    // Format the new path by concatenating the destination directory and the relative filepath
    snprintf(new_path, sizeof(new_path), "%s/%s", destination_directory, filepath + strlen(source_directory) + 1);

    if (flag_type == FTW_D){ // Check if the current file type is a directory
        // Create the directory at the new path with the same permissions
        if (mkdir(new_path, sb->st_mode) < 0 && errno != EEXIST){
            return -1; // Return an error if directory creation fails for any other reason
        }
    }
    // Check if the current file type is a regular file
    else if (flag_type == FTW_F){
        // If copying fails or removing the original file fails, return an error
        if (copyFile(filepath, new_path) < 0 || unlink(filepath) < 0){
            return -1;
        }
    }
    return 0;
}


// Recursive function to remove a directory and its contents
int remove_directory(const char *source_path)
{
    struct stat statbuf;
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    // Error Check
    if (stat(source_path, &statbuf) < 0){
        return -1;
    }
    // Check if the source path is not a directory
    if (!S_ISDIR(statbuf.st_mode)){
        return remove(source_path); // If it's a file, remove it and return the result
    }
    // To open the directory
    dir = opendir(source_path);
    //Error Check
    if (!dir){ 
        return -1;
    }
    // Iterate over each entry in the directory
    while ((entry = readdir(dir)) != NULL){
        // Skip the current directory (.) and the parent directory (..)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        // Construct the full path of the entry
        snprintf(path, sizeof(path), "%s/%s", source_path, entry->d_name);

        // Recursively remove the entry (file or directory)
        if (remove_directory(path) < 0){
            closedir(dir); // Close the directory before returning an error
            return -1;     // Return an error code
        }
    }
    closedir(dir);// Close the directory
    return rmdir(source_path); // Remove the directory itself after its contents have been removed
}

// Function to print the usage message
void display_usage(const char *program_name){
    // Prints a formatted usage message to the standard error stream
    fprintf(stderr, " Incorrect Arguments. \n  Correct use: %s [-nf | -nd | -sf] [root_dir]\n"
                    "       %s -cpx [source_dir] [destination_dir] [file_extension]\n"
                    "       %s -mv [source_dir] [destination_dir]\n",
            program_name, program_name, program_name);
}

// Main Function
int main(int argc, char *argv[])
{
    if (argc < 2){ // Check if there are enough arguments
        display_usage(argv[0]);
        return 1;
    }

    const char *flag = argv[1]; // Get the flag

    switch (flag[1]){
    // Case to Handle the count total number of files and directories -nf and -nd
    case 'n':
        if (argc != 3){ // Check if the number of arguments is correct
            display_usage(argv[0]); // Call the usage function
            return 1;
        }
        if (nftw(argv[2], count_items, 20, 0) == -1){ // Traverse the directory tree
            exit(1);
        }
        printf("Total number of %s: %d\n", flag[2] == 'f' ? "files" : "directories", flag[2] == 'f' ? file_count : directory_count);
        break;

    // Case to Handle the size of files present into the directory using  -sf
    case 's':
        if (argc != 3){// Check if the number of arguments is correct
            display_usage(argv[0]); // Call the usage function
            return 1;
        }
        if (nftw(argv[2], size_of_files, 20, 0) == -1){ // Traverse the directory tree
            exit(1);
        }
        break;

    // Case to handle the copy files from source to destination using -cpx
    case 'c':
        if (argc < 4 || argc > 5){ // Check if the number of arguments is correct
            display_usage(argv[0]);
            return 1;
        }
        strncpy(source_directory, argv[2], sizeof(source_directory) - 1);// Copy source directory path
        strncpy(destination_directory, argv[3], sizeof(destination_directory) - 1); // Copy destination directory path
        extension = (argc == 5) ? argv[4] : NULL; // Set file extension if provided

        if (nftw(source_directory, copy_directory_path, 20, FTW_D) != 0){ // Traverse the source directory tree and copy files
            perror("nftw");
            exit(1);
        }
        printf("Copied contents from '%s' to '%s' excluding '%s' files.\n", source_directory, destination_directory, extension ? extension : "none");
        break;

    // Case to handle to move the files from source to destination using -mv
    case 'm':
        if (argc != 4){
            display_usage(argv[0]);
            return 1;
        }
        strncpy(source_directory, argv[2], sizeof(source_directory) - 1);
        strncpy(destination_directory, argv[3], sizeof(destination_directory) - 1);

        // Move files and directories
        if (nftw(source_directory, move_file, 20, FTW_D) != 0){
            exit(1);
        }
       
        // Remove the source directory after moving contents
        if (nftw(source_directory, remove_directory, 20, FTW_D) != 0){
            exit(1);
        }
        printf("Moved contents from '%s' to '%s'.\n", source_directory, destination_directory);
        break;

    default:
        display_usage(argv[0]); // Print usage message if the flag is unrecognized
        return 1;
    }
    return 0;
}
