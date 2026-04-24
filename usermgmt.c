#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ANSI-color-codes.h"
#include "macros.h"

#include "usermgmt.h"
#include "environment.h"


static char current_user[MAX_USERNAME_LEN] = "";

static void get_session_path(char* buf, size_t len)
{
    const char* home = getenv("HOME");
    if (home == NULL) {
        home = "/tmp";
    }

    char dir[512];
    snprintf(dir, sizeof(dir), "%s" ISOSH_DIR, home);
    mkdir(dir, 0700);

    snprintf(buf, len, "%s%s/%s", home, ISOSH_DIR, USER_SESSION_FILENAME);
}

// saves current user to session file for persistence
static void save_session()
{
    char path[512];
    get_session_path(path, sizeof(path));

    FILE* f = fopen(path, "w");
    if (f == NULL) return;
    fprintf(f, "%s\n", current_user);
    fclose(f);
}

// removes session file to log out user
static void clear_session()
{
    char path[512];
    get_session_path(path, sizeof(path));
    remove(path);
}

void load_user_session()
{
    char path[512];
    get_session_path(path, sizeof(path));

    FILE* f = fopen(path, "r");
    if (f == NULL) return;

    char buf[MAX_USERNAME_LEN];
    if (fgets(buf, sizeof(buf), f) != NULL) {
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] != '\0') {
            strncpy(current_user, buf, MAX_USERNAME_LEN - 1);
            current_user[MAX_USERNAME_LEN - 1] = '\0';
        }
    }
    fclose(f);
}

char* get_current_user()
{
    if (current_user[0] == '\0') {
        return NULL;
    }
    return current_user;
}

// wrapper for getting the current user to use in builtins.c
int handle_user_cmd(char** parsedCmd){
    printf(BYEL "Current user: '%s'\n" CRESET, get_current_user() ? get_current_user() : "None");
    return 0;
}

int login_user(char* username)
{
    if (strlen(username) >= MAX_USERNAME_LEN) {
        fprintf(stderr, "login: username too long (max %d chars)\n", MAX_USERNAME_LEN - 1);
        return 1;
    }

    if (current_user[0] != '\0') {
        logout_user();
    }

    strncpy(current_user, username, MAX_USERNAME_LEN - 1);
    current_user[MAX_USERNAME_LEN - 1] = '\0';
    save_session();
    load_user_variables(current_user);

    printf(BGRN "Logged in as '%s'.\n" CRESET, current_user);
    return 0;
}

int handle_login_user(char** parsedCmd)
{
    if (parsedCmd[1] == NULL) {
        fprintf(stderr, "usage: login <username>\n");
        return 1;
    }

    return login_user(parsedCmd[1]);
}

int logout_user()
{
    if (current_user[0] == '\0') {
        printf(BYEL "No user logged in.\n" CRESET);
        return 1;
    }

    printf(BGRN "Logged out user '%s'.\n" CRESET, current_user);
    current_user[0] = '\0';
    clear_session();
    return 0;
}

int handle_logout_user(char** parsedCmd)
{
    return logout_user();
}
