#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "macros.h"
#include "environment.h"
#include "usermgmt.h"

typedef struct {
    char name[MAX_VAR_NAME_LEN];
    char value[MAX_VAR_VALUE_LEN];
} UserVar;

static UserVar user_vars[MAX_USER_VARS];
static int num_user_vars = 0;

static void get_user_vars_path(const char* username, char* buf, size_t len)
{
    const char* home = getenv("HOME");
    if (home == NULL) home = "/tmp";
    snprintf(buf, len, "%s" ISOSH_DIR "/%s" USER_VARS_FILENAME, home, username);
}

// in-memory only
static void store_variable(const char* name, const char* value)
{
    for (int i = 0; i < num_user_vars; i++) {
        if (strcmp(user_vars[i].name, name) == 0) {
            strncpy(user_vars[i].value, value, MAX_VAR_VALUE_LEN - 1);
            user_vars[i].value[MAX_VAR_VALUE_LEN - 1] = '\0';
            return;
        }
    }

    if (num_user_vars >= MAX_USER_VARS) {
        fprintf(stderr, "setvar: max variable limit (%d) reached\n", MAX_USER_VARS);
        return;
    }

    strncpy(user_vars[num_user_vars].name, name, MAX_VAR_NAME_LEN - 1);
    user_vars[num_user_vars].name[MAX_VAR_NAME_LEN - 1] = '\0';
    strncpy(user_vars[num_user_vars].value, value, MAX_VAR_VALUE_LEN - 1);
    user_vars[num_user_vars].value[MAX_VAR_VALUE_LEN - 1] = '\0';
    num_user_vars++;
}

char* get_user_variable(const char* name)
{
    for (int i = 0; i < num_user_vars; i++) {
        if (strcmp(user_vars[i].name, name) == 0) {
            return user_vars[i].value;
        }
    }
    return NULL;
}

void save_user_variables(const char* username)
{
    if (username == NULL) return;

    const char* home = getenv("HOME");
    if (home == NULL) home = "/tmp";

    // ensure ~/.isosh/ and ~/.isosh/<username>/ exist
    char dir[512];
    snprintf(dir, sizeof(dir), "%s" ISOSH_DIR, home);
    mkdir(dir, 0700);
    snprintf(dir, sizeof(dir), "%s" ISOSH_DIR "/%s", home, username);
    mkdir(dir, 0700);

    char path[512];
    get_user_vars_path(username, path, sizeof(path));

    FILE* f = fopen(path, "w");
    if (f == NULL) {
        perror("save_user_variables");
        return;
    }

    for (int i = 0; i < num_user_vars; i++) {
        fprintf(f, "%s=%s\n", user_vars[i].name, user_vars[i].value);
    }
    fclose(f);
}

void set_user_variable(const char* name, const char* value)
{
    store_variable(name, value);
    save_user_variables(get_current_user());
}

void load_user_variables(const char* username)
{
    if (username == NULL) return;

    // clear existing in-memory vars before loading
    num_user_vars = 0;

    char path[512];
    get_user_vars_path(username, path, sizeof(path));

    FILE* f = fopen(path, "r");
    if (f == NULL) return;

    char line[MAX_VAR_NAME_LEN + MAX_VAR_VALUE_LEN + 2];
    while (fgets(line, sizeof(line), f) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        char* eq = strchr(line, '=');
        if (eq == NULL) continue;
        *eq = '\0';
        store_variable(line, eq + 1);
    }

    fclose(f);
    DEBUG_PRINT("Loaded %d user variable(s) for '%s'\n", num_user_vars, username);
}

// returns value or NULL if not set, checks user vars first, then OS env
char* get_environment_variable(char* name)
{
    if (name == NULL) return NULL;

    const char* key = (name[0] == ENV_VAR_PREFIX) ? name + 1 : name;

    char* user_var = get_user_variable(key);
    if (user_var != NULL) {
        DEBUG_PRINT("User variable '%s': %s\n", key, user_var);
        return user_var;
    }

    char* env_var = getenv(key);
    DEBUG_PRINT("Environment variable '%s': %s\n", key, env_var ? env_var : "Not set");
    return env_var;
}

// expands all $VAR occurrences to handles embedded vars like "$PATH:/bin"
void expand_variables(const char* input, char* out, size_t out_len)
{
    size_t out_pos = 0;
    const char* p = input;

    while (*p != '\0' && out_pos < out_len - 1) {
        if (*p != ENV_VAR_PREFIX) {
            out[out_pos++] = *p++;
            continue;
        }

        p++; // skip '$'

        char varname[MAX_VAR_NAME_LEN];
        int vlen = 0;
        while (*p != '\0' && (isalnum((unsigned char)*p) || *p == '_') && vlen < MAX_VAR_NAME_LEN - 1) {
            varname[vlen++] = *p++;
        }
        varname[vlen] = '\0';

        if (vlen == 0) {
            // '$' with no valid name, treat as literal '$'
            if (out_pos < out_len - 1) out[out_pos++] = '$';
            continue;
        }

        char* val = get_environment_variable(varname);
        if (val != NULL) {
            size_t to_copy = strlen(val);
            if (out_pos + to_copy > out_len - 1) to_copy = out_len - 1 - out_pos;
            memcpy(out + out_pos, val, to_copy);
            out_pos += to_copy;
        }
        // unset var expands to nothing
    }
    out[out_pos] = '\0';
}

int handle_setuservar(char** parsedCmd)
{
    if (parsedCmd[1] == NULL || parsedCmd[2] == NULL) {
        fprintf(stderr, "usage: setvar <name> <value>\n");
        return 1;
    }

    if (get_current_user() == NULL) {
        fprintf(stderr, "setvar: no user logged in\n");
        return 1;
    }

    set_user_variable(parsedCmd[1], parsedCmd[2]);
    printf("Set '%s' = '%s'\n", parsedCmd[1], parsedCmd[2]);
    return 0;
}

int handle_clearuservar(char** parsedCmd)
{
    if (parsedCmd[1] == NULL) {
        fprintf(stderr, "usage: clearvar <name>\n");
        return 1;
    }

    if (get_current_user() == NULL) {
        fprintf(stderr, "clearvar: no user logged in\n");
        return 1;
    }

    const char* name = parsedCmd[1];
    for (int i = 0; i < num_user_vars; i++) {
        if (strcmp(user_vars[i].name, name) == 0) {
            // shift remaining entries down
            for (int j = i; j < num_user_vars - 1; j++) {
                user_vars[j] = user_vars[j + 1];
            }
            num_user_vars--;
            save_user_variables(get_current_user());
            printf("Cleared '%s'\n", name);
            return 0;
        }
    }

    fprintf(stderr, "clearvar: variable '%s' not found\n", name);
    return 1;
}
