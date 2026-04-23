#define _DEFAULT_SOURCE // base UNIX functionality

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "builtins.h"
#include "macros.h"
#include "usermgmt.h"
#include "environment.h"

#include "ANSI-color-codes.h"  // enables colored output

// --- BUILTIN COMMAND HANDLERS --- //

// REMEMBER TO UPDATE BUILTINS AND NUM_BUILTINS IN BUILTINS.H WHEN ADDING NEW BUILTINS

// builtin command handler function declarations
int handle_cd(char** parsedCmd);
int handle_clear(char** parsedCmd);
int print_help(char** parsedCmd);
int handle_tree(char** parsedCmd);
int handle_newdir(char** parsedCmd);
int handle_newf(char** parsedCmd);
int handle_exit(char** parsedCmd);

// builtin command handler mapping
int (*builtin_func[]) (char **) = {
    &handle_cd,
    &handle_clear,
    &print_help,
    &handle_tree,
    &handle_newdir,
    &handle_newf,
    &handle_user_cmd,
    &handle_login_user,
    &handle_logout_user,
    &handle_setuservar,
    &handle_clearuservar,
    &handle_exit
};

// --- EXPORTED FUNCTIONS --- //

// checks if command is a builtin and return index, or -1 if not a builtin
int determine_builtin_command(char** parsedCmd)
{
    for (int i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(parsedCmd[0], BUILTINS[i]) == 0) {
            DEBUG_PRINT("Builtin command detected: %s\n", BUILTINS[i]);
            return i;
        }
    }

    return -1; // not a builtin command
}

int execute_builtin_command(int builtinIndex, char** parsedCmd)
{
    int i = 0;
    while (parsedCmd[i] != NULL){
        DEBUG_PRINT("Arg %d: %s\n", i, parsedCmd[i]);
        i++;
    }
    // call the handler via mapped function
    return (*builtin_func[builtinIndex])(parsedCmd);
}

// --- END EXPORTED FUNCTIONS --- //

// --- HANDLERS --- //

int handle_cd(char** parsedCmd)
{    
    if (parsedCmd[1] == NULL) {
        fprintf(stderr, "expected argument to \"cd\"\n");
        return 1;
    } else {
        if (chdir(parsedCmd[1]) != 0) {
            perror("handle cd");
            return 1;
        }
    }
    return 0;
}

int handle_clear(char** parsedCmd)
{
    clear();
    return 0;
}

int print_help(char** parsedCmd)
{
    printf(BGRN "--- Isosh Help ---\n" CRESET);
    printf(BYEL "Built-in commands:\n" CRESET);
    for (int i = 0; i < NUM_BUILTINS; i++) {
        printf("  - %s\n", BUILTINS[i]);
    }

    printf(BYEL "\nPipe commands using \"|\" character." CRESET);
    printf("\n  - Example: ls | grep *.txt\n");

    printf(BYEL "\nFor more information on these commands, refer to the README.\n" CRESET);
    return 0;
}

int handle_exit(char** parsedCmd)
{
    return -1; // signal to exit shell
}

int file_is_hidden(char* filename, int show_all){
    if (show_all){
        return 0;
    }

    // DEBUG_PRINT("Filename %s\n", filename);
    if (filename != NULL){
        if (filename[0] == '.'){
            return 1;
        }
    }
    return 0;
}

// helper for tree command, recursively prints directory contents
void show_dir_content(char * path, int depth[], int max_depth, int show_all)
{
    DIR * d = opendir(path); // open the path
    if(d==NULL) return; // if was not able, return

    // copy depth arrays to local arrays for this recursive call
    int local_depth[16];
    int next_depth[16];
    
    memcpy(local_depth, depth, 16 * sizeof(depth[0]));
    memcpy(next_depth, depth, 16 * sizeof(depth[0]));

    // find the current depth of the recursive call
    int depth_val = -1;
    for (int i = 0; i < 16; i++) {
        if (local_depth[i] == 1) {
            depth_val = i;
        }
    }

    // DEBUG_PRINT("Depth %d\n", depth_val);

    if (depth_val == -1 || depth_val > max_depth) {
        return;
    }
    next_depth[depth_val+1] = 1;
    

    // create indent string based on depth
    char *indent_prefix = "│   ";
    char *indent_footer = "├── ";
    char *indent_last_footer = "└── ";
    char *indent_last_prefix = "    ";

    char indent[256] = "";
    char indent_last[256] = "";

    for (int i = 0; i < depth_val; i++) {
        if (local_depth[i] == 1) {
            strcat(indent, indent_prefix);
            strcat(indent_last, indent_prefix);
        }
        else if (local_depth[i] == 2) {
            strcat(indent, indent_last_prefix);
            strcat(indent_last, indent_last_prefix);
        }
    }

    strcat(indent, indent_footer);
    strcat(indent_last, indent_last_footer);

    char *indent_str = indent;

    // count total number of files in the directory for formatting purposes
    int file_count = 0;
    struct dirent * fcdir;
    while ((fcdir = readdir(d)) != NULL) {
        if (file_is_hidden(fcdir->d_name, show_all)){
            continue;
        }
        file_count++;
    }

    rewinddir(d);

    // itterate through directory entries
    struct dirent * dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL)
    {
        if(file_is_hidden(dir->d_name, show_all)){
            DEBUG_PRINT("file/dir \"%s\" hidden\n", dir->d_name);
            continue;
        }
        count++;
        if (count == file_count) 
        {
            local_depth[depth_val] = 2;
            next_depth[depth_val] = 2;
            indent_str = indent_last;
        }

        // type is not directory
        if(dir-> d_type != DT_DIR)
        { 
            printf(CYN "%s%s" CRESET "\n", indent_str, dir->d_name);
        }
        // type is directory (and not . or ..)
        else if(dir -> d_type == DT_DIR)
        {
            printf(CYN "%s" YEL "%s" CRESET "\n", indent_str, dir->d_name);
            
            // skip itterating through . and .. directories
            if(strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0){
                //DEBUG_PRINT("Skipping iter over %s\n", dir->d_name);
                continue;
            }
            
            char d_path[257];
            sprintf(d_path, "%s/%s", path, dir->d_name);
            show_dir_content(d_path, next_depth, max_depth, show_all); // recurse with the new path
        }
    }

    closedir(d); // close the directory
}

// print directory tree from given path or current directory if no path provided
int handle_tree(char** parsedCmd)
{
    int max_depth = 1; // default max depth is 1 (this directory and one level of subdirectories)
    int depths[16] = {0};
    depths[0] = 1;

    int starting_dir_idx = -1;
    int show_all = 0;
    int i = 0;
    while (parsedCmd[i] != NULL){
        if (parsedCmd[i][0] == '-'){
            if (strcmp(parsedCmd[i], "-a") == 0 || strcmp(parsedCmd[i], "-A") == 0) {
                show_all = 1;
            } else if (strcmp(parsedCmd[i], "-d") == 0 || strcmp(parsedCmd[i], "-D") == 0) {
                i++;
                if (parsedCmd[i] == NULL){
                    printf(BRED "ERROR: " CRESET "-d requires a depth of 0-15. -d <depth>");
                }
                max_depth = atoi(parsedCmd[i]);
            } else {
                printf(BYEL "Warning: " CRESET "argument \"%s\" not known. Ignoring.\n", parsedCmd[i]);
            }
        } else {
            starting_dir_idx = i;
        }

        i++;
    }

    if (show_all){
        DEBUG_PRINT("Showing all files\n");
    }



    if (starting_dir_idx > 0) {
        show_dir_content(parsedCmd[starting_dir_idx], depths, max_depth, show_all);
    } else {
        // if no argument provided, use local cwd
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd() error");
            return 1;
        }
        
        show_dir_content(cwd, depths, max_depth, show_all);
    }
    
    return 0;
}

int handle_newdir(char** parsedCmd){

    if (parsedCmd[1] == NULL) {
        fprintf(stderr, "expected argument to \"newdir\"\n");
        return 1;
    }

    int numElements = sizeof(parsedCmd) / sizeof(parsedCmd[0]);

    mode_t mode = 0777; // default permissions IN OCTAL

    if (numElements >= 3) {
        if (parsedCmd[2] != NULL){
            mode = strtol(parsedCmd[2], NULL, 8); // convert string to octal
            DEBUG_PRINT("Mode provided: %o\n", mode);
        }
    }

    if (mkdir(parsedCmd[1], mode) != 0) {
        perror("newdir");
        return 1;
    }

    printf("Directory created: %s\n", parsedCmd[1]);

    handle_cd(parsedCmd); // change into the new directory

    return 0;
}

int handle_newf(char** parsedCmd) {
    FILE *fptr;

    fptr = fopen(parsedCmd[1], "w");

    if (fptr == NULL) {
        printf("newf: failed to create file\n");
        return 1;
    }

    fclose(fptr);

    printf("File '%s' created successfully.\n", parsedCmd[1]);
    return 0;
}

// --- END HANDLERS --- //
