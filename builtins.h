#ifndef BUILTINS_H
#define BUILTINS_H

// List of builtin commands
#define BUILTINS ((const char*[]){ "cd", "clear", "help", "tree", "newdir", "newf", "user", "login", "logout", "setuservar", "clearuservar", "exit"})
#define NUM_BUILTINS (int)(sizeof(BUILTINS) / sizeof(char*))

int determine_builtin_command(char** parsedCmd);
int execute_builtin_command(int builtinIndex, char** parsedCmd);

#endif
