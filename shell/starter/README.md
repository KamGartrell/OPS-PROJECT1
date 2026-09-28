# Shell

A shell interface with the OS that parses the user's input into a format for the OS to read and execute commands.
The shell also handles complex operations like I/O redirection, piping, and background processing. 

## Group Members
- **Kameryn Gartrell**: ksg22b@fsu.edu

## Division of Labor

### Part 1: Prompt
- **Responsibilities**: Creating a prompt system using environment variables
- **Assigned to**: Kameryn Gartrell

### Part 2: Environment Variables
- **Responsibilities**: Replaces any token starting with $ with its environment variable
- **Assigned to**: Kameryn Gartrell

### Part 3: Tilde Expansion
- **Responsibilities**: Expands standalone ~ or ~/ prefixes to the user's absolute home directory path
- **Assigned to**: Kameryn Gartrell

### Part 4: $PATH Search
- **Responsibilities**: Searches the directories in the PATH environment variable for executables
- **Assigned to**: Kameryn Gartrell

### Part 5: External Command Execution
- **Responsibilities**: Forks a child process and uses execv to run external commands
- **Assigned to**: Kameryn Gartrell

### Part 6: I/O Redirection
- **Responsibilities**: Handles standard input and output file redirection using <, >, and >>
- **Assigned to**: Kameryn Gartrell

### Part 7: Piping
- **Responsibilities**: Connects the output of one command to the input of another using pipes
- **Assigned to**: Kameryn Gartrell

### Part 8: Background Processing
- **Responsibilities**: Detects the & token and tracks background jobs without blocking the shell
- **Assigned to**: Kameryn Gartrell

### Part 9: Internal Command Execution
- **Responsibilities**: Executes built-in commands for exit, jobs, and cd directly in the shell
- **Assigned to**: Kameryn Gartrell

## File Listing
```
shell/starter/
│
├── src/
│ └── lexer.c
│
├── include/
│ └── lexer.h
│
├── README.md
└── Makefile
```
## How to Compile & Execute

### Requirements
- **Compiler**: `gcc` for C/C++
- **Dependencies**: unistd.h, sys/wait.h, fcntl.h

### Compilation

make

This will build the executable in bin/shell
### Execution

./bin/shell

This will run the program.

## Development Log
Each member records their contributions here.

### Kameryn Gartrell

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-09-20 | Implemented Parts 1-4  |
| 2026-09-25 | Implemented Parts 5-7  |
| 2026-09-27 | Implemented Parts 8-9  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

- NONE

## Bugs
- **Bug 1**: Sort / cat will hang the shell if there are no arguments or inputs

## Considerations
 - Github Copilot helped with syntax formatting and documentation
 - Google Gemnii helped with documentation and help with archiecture/concepts of project