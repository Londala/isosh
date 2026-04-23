#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "globals.h"

#define ENV_VAR_PREFIX '$'
#define USER_VARS_FILENAME "/user_variables"
#define MAX_USER_VARS 64
#define MAX_VAR_NAME_LEN 64
#define MAX_VAR_VALUE_LEN 256

char* get_environment_variable(char* name);
char* get_user_variable(const char* name);
void set_user_variable(const char* name, const char* value);
void load_user_variables(const char* username);
void save_user_variables(const char* username);
void expand_variables(const char* input, char* out, size_t out_len);

int handle_setuservar(char** parsedCmd);
int handle_clearuservar(char** parsedCmd);

#endif
