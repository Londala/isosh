#define _DEFAULT_SOURCE // base UNIX functionality

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <linux/limits.h>  // for getcwd and PATH_MAX
#include <readline/readline.h>  // for readline
#include <readline/history.h>  // for arrow key use for history

// enables colored output
#include "ANSI-color-codes.h"
#include "macros.h"
#include "builtins.h"
#include "usermgmt.h"
#include "environment.h"

#define MAXLINELEN 1000 // max letters per line
#define MAXNUMCMD 100 // max commands per line

// terminal line prefix and cmd separator
char* defaultPrefix = YEL "isosh" CRESET;
char* terminalSep = YEL "$" CRESET;

// --- INTERNAL SHELL FUNCTIONS --- //

// prints welcome message when user opens the shell
static void print_welcome_message()
{
    clear();
    printf(BYEL "Welcome to Isosh!\n" CRESET);
    DEBUG_PRINT("Number of built-in commands: %d\n", NUM_BUILTINS);
    DEBUG_PRINT("debug\n" BRED "*** DEBUG LOGGING ENABLED ***" CRESET "\n");

    char* user = get_current_user();
    if (user) {
        printf(BGRN "Logged in as '%s'.\n" CRESET, user);
    } else {
        printf(BYEL "No user logged in.\n" CRESET);
    }
}

// prints exit message when user exits the shell
static void print_exit_message()
{
    printf(BYEL "Exiting isosh...\n" CRESET);
}

// returns the line prefix string with terminal name and current directory
static char* get_line_prefix()
{
    static char line_prefix[PATH_MAX + 256];

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    char* prefix = defaultPrefix;

    char* user = get_current_user();
    if (user != NULL) {
        prefix = user;
    }

    snprintf(line_prefix, sizeof(line_prefix),
        BLU "%s " YEL "[" CYN "%s" YEL "] %s " CRESET,
        prefix, cwd, terminalSep);
    return line_prefix;
}

// --- END INTERNAL SHELL FUNCTIONS --- //

// --- COMMAND PARSING --- //

// read line from user input and store in str
static int read_line(char* str)
{
    char* buf;

    buf = readline(get_line_prefix());
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}

// splits command into piped commands if applicable
static int parse_pipe(char* str, char** pipedCmds)
{
    for (int i = 0; i < 2; i++) {
        pipedCmds[i] = strsep(&str, "|");
        if (pipedCmds[i] == NULL) {
            break;
        }
    }

    if (pipedCmds[1] == NULL) {
        return 0; // not piped
    }

    return 1; // piped
}

// parse line into command words
static void parse_commands(char* str, char** parsedCmd)
{
    for (int i = 0; i < MAXNUMCMD; i++) {
        // parse command into arguments
        // removes spaces, tabs, newlines, carriage returns, and alert chars
        parsedCmd[i] = strsep(&str, " \t\n\r\a");
        if (parsedCmd[i] == NULL) {
            break;
        }
        if (strlen(parsedCmd[i]) == 0) {
            i--;
        }
        if (strchr(parsedCmd[i], ENV_VAR_PREFIX) != NULL) {
            char expanded[MAXLINELEN];
            expand_variables(parsedCmd[i], expanded, sizeof(expanded));
            strncpy(parsedCmd[i], expanded, MAXLINELEN - 1);
            parsedCmd[i][MAXLINELEN - 1] = '\0';
        }
    }
}

// breaks down input into commands with arguments
static int parse_input(char* input, char** parsedCmd, char** pipedCmd)
{
    char* pipedCmds[2];

    // determine if command being piped
    int piped = parse_pipe(input, pipedCmds);

    if (piped) {
        DEBUG_PRINT("Pipe found: %s | %s\n", pipedCmds[0], pipedCmds[1]);
        // parse piped command
        parse_commands(pipedCmds[0], parsedCmd);
        // parse command being piped to
        parse_commands(pipedCmds[1], pipedCmd);
    } else {
        // parse normal command
        parse_commands(input, parsedCmd);
    }

    // TODO: user command ownership
    // if (!user_owns_command(parsedCmd)) {}

    return piped; // 0 if not piped, 1 if piped
}

// --- END COMMAND PARSING --- //

// --- COMMAND EXECUTION --- //

// execute normal command
static int execute_command(char** parsedCmd)
{
    pid_t pid;

    // fork process
    pid = fork();

    if (pid == -1){
        perror("Fork failed");
        exit(EXIT_FAILURE);
        return 1;
    } else if (pid == 0) {
        // child process
        if (execvp(parsedCmd[0], parsedCmd) < 0) {
            perror("Execution failed");
            exit(EXIT_FAILURE);
            return 1;
        }
    } else {
        // parent process
        wait(NULL);
    }

    return 0; // continue execution
}

// execute piped command
static int execute_piped_command(char** parsedCmd, char** pipedCmd)
{
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        return 1; // exit shell on pipe failure
    }

    pid1 = fork();
    if (pid1 == -1) {
        perror("Fork failed");
        return 1; // exit shell on fork failure
    } else if (pid1 == 0) {
        // first child process
        close(pipefd[0]); // close read end
        dup2(pipefd[1], STDOUT_FILENO); // redirect stdout to write end
        close(pipefd[1]);

        if (execvp(parsedCmd[0], parsedCmd) < 0) {
            perror("Child 1 execution failed");
            exit(EXIT_FAILURE);
            return 1;
        }
        exit(EXIT_SUCCESS);
        return 0;
    } 

    pid2 = fork();
    if (pid2 == -1) {
        perror("Fork failed");
        return 1; // exit shell on fork failure
    } else if (pid2 == 0) {
        // second child process
        close(pipefd[1]); // close write end
        dup2(pipefd[0], STDIN_FILENO); // redirect stdin to read end
        close(pipefd[0]);

        if (execvp(pipedCmd[0], pipedCmd) < 0) {
            perror("Child 2 execution failed");
            exit(EXIT_FAILURE);
            return 1;
        }
        exit(EXIT_SUCCESS);
        return 0;
    }
    
    // parent process
    // close both ends of the pipe in the parent process
    close(pipefd[1]);
    close(pipefd[0]);

    // wait for children
    wait(NULL);
    DEBUG_PRINT("First child done\n");
    wait(NULL);
    DEBUG_PRINT("Second child done\n");

    return 0; // continue execution
}

// main loop of the shell
static void shell_loop()
{
    char inputStr[MAXLINELEN];
    char *parsedCmd[MAXNUMCMD];
    char *pipedCmd[MAXNUMCMD];

    int status, piped, builtinIndex;

    do {
        if (read_line(inputStr)) {
            continue;
        }

        piped = parse_input(inputStr, parsedCmd, pipedCmd);

        // see builtins.h
        builtinIndex = determine_builtin_command(parsedCmd);

        if (builtinIndex != -1) {
            // execute builtin command (builtins.h)
            status = execute_builtin_command(builtinIndex, parsedCmd);
        } else if (piped) {
            // execute piped command
            DEBUG_PRINT("Executing piped command: %s | %s\n", parsedCmd[0], pipedCmd[0]);
            status = execute_piped_command(parsedCmd, pipedCmd);
        } else {
            // execute normal command
            DEBUG_PRINT("Executing command: %s\n", parsedCmd[0]);
            status = execute_command(parsedCmd);
        }

    } while (status >= 0);
    return;
}

// --- END COMMAND EXECUTION --- //

int main(int argc, char **argv)
{
    // load the user session for persistence
    load_user_session();
    load_user_variables(get_current_user());

    // TODO: Load config files
    // load_config();

    // print welcome message
    print_welcome_message();

    // Run command loop.
    shell_loop();

    // print exit message
    print_exit_message();

    // Perform any shutdown/cleanup.
    return EXIT_SUCCESS;
}
