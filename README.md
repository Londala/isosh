# ISOSH

A shell written for Unix based systems, for isolating custom environments of different users.

## Running the shell

Command to start is simply:

```
./isosh
```

## Compiling

Requires gcc and the GNU readline library (or some other library with similar functionality).

```
sudo apt install gcc readline-dev

// compile for normal use
gcc *.c -o isosh -lreadline

// compile with debug output
gcc *.c -o isosh -lreadline -DDEBUG=3 
```

## Built-in Utilities and Commands

These commands are built into the terminal directly.

`<>` args are required. `[]` args are optional.

Basic commands:

- `clear`
- `help`
- `exit`

File System Utilities:

- `cd <path>`
- `tree [path] [-a] [-d {0-15}]`
  - prints the contents of the specified directory (or the current working directory if none specified)
  - `-a` shows hidden files and dirs as well (does not walk through `.` or `..` to prevent recursion)
  - `-d` specifies the depth of subdirectories walked through. Range: 0-15. Default: 1 (current directory and one level of subdirectories)
- `newdir <path> [mode]`
  - mode is the permision mode in octal. Default: 0777
  - path is relative to current directory unless it starts with a `/`
- `newf <name>`

User Management Commands:

- `user`
- `login <username>`
- `logout`
- `setuservar <varname> <value>`
- `clearuservar <varname>`


## Piping

Pipe commands using `|` character.

Example: `ls | grep *.txt`

## Users

User commands are relatively straight forward. `user` prints the user's name, login with `login`, logout with `logout`. The user's name is also shown at the beginning of the prompt.

The logged in user is persisted between sessions, and stored in `~/.isosh/session`.

Users also can define their own variables using `setuservar`. These are treated like environment variables and take priority when being called. `clearuservar` 

These variables are also expandable, and can refrence themselves when being set (though a non alphabetic character is required after the variable name).

Ex. `$PATH=/some/dir` -> `setuservar PATH $PATH:/another/path` -> `$PATH=/some/dir:/another/path`

## File System Navigation

Moving through the file system is done with the `cd` command.

The files in a directory can be printed using the `tree` command or the standard terminal list command (usually `ls`).

Make new directories with `newdir`, and new files with `newf`. They default to standard permissions, but these can be overridden by setting the `[mode]` argument.


