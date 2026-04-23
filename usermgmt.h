#ifndef USERMGMT_H
#define USERMGMT_H

#include "globals.h"

#define MAX_USERNAME_LEN 64
#define USER_SESSION_FILENAME "saved_session"

char* get_current_user();
int login_user(char* username);
int logout_user();
void load_user_session();

// wrapper for getting the current user to use in builtins.c
int handle_user_cmd(char** parsedCmd);
int handle_login_user(char** parsedCmd);
int handle_logout_user(char** parsedCmd);

#endif
