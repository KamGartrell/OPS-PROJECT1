#define _GNU_SOURCE // For setenv()

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // for access(), chdir(), getcwd()
#include <stdbool.h>  // for bool type
#include <sys/wait.h> // for waitpid()
#include <fcntl.h>    // for file control flags like O_RDONLY, O_WRONLY

// Part 8: Background Processing
typedef struct {
	int jobId;
    char commandLine[256];
    pid_t pid; // Process ID of the background job
    bool active;
} BackgroundJob; // Structure to hold background job information

BackgroundJob bgJobs[10];
int nextJobId = 1;

// Part 9: Internal Command Execution
char history[3][256]; // Store the last three valid commands
int historyCount = 0; // Count of valid commands stored in history

int main() {
	while (1) {
		// Part 8: Routinely check if any background processes have finished
		for (int i = 0; i < 10; i++) {
			if (bgJobs[i].active) {
				int status;
				if (waitpid(bgJobs[i].pid, &status, WNOHANG) > 0) {
					printf("[%d] + done %s\n", bgJobs[i].jobId, bgJobs[i].commandLine);
					bgJobs[i].active = false;
				}
			}
		}
		
		// Part 1: Prompt
		char *user = getenv("USER");
		char *machine = getenv("MACHINE");
		char *pwd = getenv("PWD");

		// Error handling for null values
		if (user == NULL) 
			user = "unknown";
		if (machine == NULL) 
			machine = "unknown";
		if (pwd == NULL) 
			pwd = "unknown";
		// USER@MACHINE:PWD>
		printf("%s@%s:%s> ", user, machine, pwd);

		char *input = get_input();
		
		// Strip the newline from input for cleaner history recording
		char input_stripped[256] = "";
		if (input != NULL && strlen(input) > 0) {
			strncpy(input_stripped, input, 255);
			input_stripped[strcspn(input_stripped, "\n")] = 0; 
		}

		tokenlist *tokens = get_tokens(input);
		
		if (tokens->size > 0 && tokens->items[0] != NULL) {
			addHistory(input_stripped);
		}
		
		for (int i = 0; i < tokens->size; i++) {
			// Part 2: Environment variables
			if (tokens->items[i][0] == '$'){
				char *envName = tokens->items[i] + 1;
				char *envValue = getenv(envName);
				free(tokens->items[i]); 
				// Allocate memory for the new token based on the environment variable's value
				if (envValue != NULL) {
					tokens->items[i] = (char *)malloc(strlen(envValue) + 1);	
					strcpy(tokens->items[i], envValue);
				} else {
					tokens->items[i] = (char *)malloc(1);
            		tokens->items[i][0] = '\0';
				}
			}
			// Part 3: Tilde Expansion
			if (strcmp(tokens->items[i], "~") == 0 || strncmp(tokens->items[i], "~/", 2) == 0) {
				char *home = getenv("HOME");
				// If HOME is set, replace the token with the home directory path
				if (home != NULL) {
					char *new_token = (char *)malloc(strlen(home) + strlen(tokens->items[i]));
					strcpy(new_token, home);
					strcat(new_token, tokens->items[i] + 1);
					free(tokens->items[i]);
					tokens->items[i] = new_token;
				}
			}
		}

		// Part 8: Background Processing
		bool isBG = false;
		char fullCommand[256] = ""; 

		// Concatenate tokens to form the full command string for background job tracking
		if (tokens->size > 0 && tokens->items[0] != NULL) {
			for (int i = 0; i < tokens->size; i++) {
				strcat(fullCommand, tokens->items[i]);
				if (i < tokens->size - 1) strcat(fullCommand, " ");
			}
			// Check if the last token is "&" for background execution
			if (strcmp(tokens->items[tokens->size - 1], "&") == 0) {
				isBG = true;
				free(tokens->items[tokens->size - 1]);
				tokens->items[tokens->size - 1] = NULL;
				tokens->size--;
				if (strlen(fullCommand) >= 2) {
					fullCommand[strlen(fullCommand) - 2] = '\0';
				}
			}
		}

		// Part 9: Internal Command Execution
		if (tokens->size > 0 && tokens->items[0] != NULL) {
			// Exit Command
			if (strcmp(tokens->items[0], "exit") == 0) {
				// Wait for any active background processes
				for (int i = 0; i < 10; i++) {
					if (bgJobs[i].active) {
						waitpid(bgJobs[i].pid, NULL, 0);
					}
				}
				// Display the last three valid commands
				if (historyCount == 0) {
					printf("No valid commands to show.\n");
				} else {
					printf("Last (%d) valid commands:\n", historyCount);
					for (int i = 0; i < historyCount; i++) {
						printf("[%d]: %s\n", i + 1, history[i]);
					}
				}
				
				free(input);
				free_tokens(tokens);
				exit(0); 
			}
			
			// Jobs Command
			else if (strcmp(tokens->items[0], "jobs") == 0) {
				bool active_jobs = false;
				// Display all active background jobs
				for (int i = 0; i < 10; i++) {
					if (bgJobs[i].active) {
						printf("[%d]+ %d %s\n", bgJobs[i].jobId, bgJobs[i].pid, bgJobs[i].commandLine);
						active_jobs = true;
					}
				}
				if (!active_jobs) {
					printf("No active background processes.\n");
				}
				
				free(input);
				free_tokens(tokens);
				continue; // Skips the rest of the loop so we don't try to execv("jobs")
			}
			
			// Cd PATH Command
			else if (strcmp(tokens->items[0], "cd") == 0) {
				if (tokens->size > 2) {
					printf("cd: too many arguments\n");
				} else {
					// Use HOME if no arguments are provided, otherwise use the specified path
					char *target;

					if (tokens->size == 2) {
    					target = tokens->items[1];
					} else {
    					target = getenv("HOME");
					}
					
					// chdir changes the directory
					if (chdir(target) != 0) {
						printf("cd: %s: No such file or directory\n", target);
					} else {
						// Update PWD so the shell prompt reflects the new directory
						char currentwdir[1024];
						if (getcwd(currentwdir, sizeof(currentwdir)) != NULL) { // Get the current working directory
							setenv("PWD", currentwdir, 1);  // Update the PWD environment variable
						}
					}
				}
				free(input);
				free_tokens(tokens);
				continue; // Skips the rest of the loop
			}
		}


		// Part 7: Check for Piping before executing
		int pipeId = -1;
		for (int j = 0; j < tokens->size; j++) {
			if (tokens->items[j] != NULL && strcmp(tokens->items[j], "|") == 0) {
				pipeId = j;
				break;
			}
		}

		if (pipeId != -1) {
			// Handle piping between two commands [cmd1 | cmd2]
			tokens->items[pipeId] = NULL; 
			char **cmd1Args = tokens->items;
			char **cmd2Args = &tokens->items[pipeId + 1];

			// Create a pipe for inter-process communication
			int filedesc[2]; 
			pipe(filedesc); 

			// Fork the first child process for the first command
			pid_t pid1 = fork();
			if (pid1 == 0) {
				dup2(filedesc[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
				close(filedesc[0]);
				close(filedesc[1]);

				char *path = getAbsolutePath(cmd1Args[0]);
				if (path) 
					execv(path, cmd1Args);
				
				printf("%s: Command not found\n", cmd1Args[0]);
				exit(1);
			}

			// Fork the second child process for the second command
			pid_t pid2 = fork();
			if (pid2 == 0) {
				dup2(filedesc[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
				close(filedesc[0]);
				close(filedesc[1]);

				char *path = getAbsolutePath(cmd2Args[0]);
				if (path) 
					execv(path, cmd2Args);
				
				printf("%s: Command not found\n", cmd2Args[0]);
				exit(1);
			}

			// Parent Process
			close(filedesc[0]);
			close(filedesc[1]);
			if (isBG) {
				// Record the background job for the second command in the pipe
				for (int i = 0; i < 10; i++) {
					if (!bgJobs[i].active) {
						bgJobs[i].pid = pid2; 
						bgJobs[i].jobId = nextJobId++;
						strcpy(bgJobs[i].commandLine, fullCommand);
						bgJobs[i].active = true;
						printf("[%d] %d\n", bgJobs[i].jobId, pid2);
						break;
					}
				}
			} else {
				// Wait for both child processes to finish if not running in the background
				waitpid(pid1, NULL, 0);
				waitpid(pid2, NULL, 0);
			}

		} else if (tokens->size > 0 && tokens->items[0] != NULL) {
			// Handle normal command execution
			char *commandPath = getAbsolutePath(tokens->items[0]);

			if (commandPath != NULL) {
				pid_t pid = fork();
				if (pid == 0) {
					// Part 6: I/O Redirection
					for (int k = 0; k < tokens->size; k++) {
						if (tokens->items[k] != NULL && strcmp(tokens->items[k], "<") == 0) {
							if (k + 1 < tokens->size) {
								// Open the input file for reading
								int inputFileDesc = open(tokens->items[k+1], O_RDONLY);

								if (inputFileDesc < 0) {
									printf("Error: Could not open input file.\n");
									exit(1);
								}
								dup2(inputFileDesc, STDIN_FILENO);
								close(inputFileDesc);
								tokens->items[k] = NULL;
							}
						} else if (tokens->items[k] != NULL && strcmp(tokens->items[k], ">") == 0) {
							if (k + 1 < tokens->size) {
								// Open the output file for writing
								int outputFileDesc = open(tokens->items[k+1], O_WRONLY | O_CREAT | O_TRUNC, 0600);

								if (outputFileDesc < 0) {
									printf("Error: Could not open output file.\n");
									exit(1);
								}
								dup2(outputFileDesc, STDOUT_FILENO);
								close(outputFileDesc);
								tokens->items[k] = NULL;
							}
						} else if (tokens->items[k] != NULL && strcmp(tokens->items[k], ">>") == 0) {
							if (k + 1 < tokens->size) {
								// Open the output file for appending
								int outputFileDesc = open(tokens->items[k+1], O_WRONLY | O_CREAT | O_APPEND, 0600);

								if (outputFileDesc < 0) {
									printf("Error: Could not open output file.\n");
									exit(1);
								}
								dup2(outputFileDesc, STDOUT_FILENO);
								close(outputFileDesc);
								tokens->items[k] = NULL;
							}
						}
					}
					execv(commandPath, tokens->items);
					printf("Error: Failed to execute %s\n", tokens->items[0]);
					exit(1);
				} else if (pid > 0) {
					// Parent Process / Normal Execution
					if (isBG) {
						// Record the background job
						for (int i = 0; i < 10; i++) {
							if (!bgJobs[i].active) {
								bgJobs[i].pid = pid; 
								bgJobs[i].jobId = nextJobId++;
								strcpy(bgJobs[i].commandLine, fullCommand);
								bgJobs[i].active = true;
								printf("[%d] %d\n", bgJobs[i].jobId, pid);
								break;
							}
						}
					} else {
						// Wait for the child process to finish if not running in the background
						int status;
						waitpid(pid, &status, 0);
					}
				} else {
					printf("Error: Failed to fork process\n");
				}
				free(commandPath); 
			} else {
				printf("%s: Command not found\n", tokens->items[0]);
			}
		}
		// Cleanup
		free(input);
		free_tokens(tokens);
	}
	return 0;
}

// Provided Helper Functions
char *get_input(void) {
	char *buffer = NULL;
	int bufsize = 0;
	char line[5];
	while (fgets(line, 5, stdin) != NULL)
	{
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;
		buffer = (char *)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;
		if (newln != NULL)
			break;
	}
	buffer = (char *)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;
	return buffer;
}

tokenlist *new_tokenlist(void) {
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **)malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item) {
	int i = tokens->size;

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *)malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

tokenlist *get_tokens(char *input) {
	char *buf = (char *)malloc(strlen(input) + 1);
	strcpy(buf, input);
	tokenlist *tokens = new_tokenlist();
	char *tok = strtok(buf, " ");
	while (tok != NULL)
	{
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}
	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens) {
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}

// Helper function to get the absolute path of a command
char* getAbsolutePath(char* command) {
    if (command == NULL) 
		return NULL;
	// If the command contains a '/', treat it as a path and return it directly
    if (strchr(command, '/') != NULL) {
        char *copy = (char *)malloc(strlen(command) + 1);
        strcpy(copy, command);
        return copy; 
    }
    
    char *envPath = getenv("PATH");
	// If PATH is not set, return NULL
    if (envPath != NULL) {
        char *pathCopy = (char *)malloc(strlen(envPath) + 1);
        strcpy(pathCopy, envPath);
        char *directory = strtok(pathCopy, ":");
        
        while (directory != NULL) {
            char *fullPath = (char *)malloc(strlen(directory) + strlen(command) + 2);
            sprintf(fullPath, "%s/%s", directory, command);
            // Check if the command is executable
            if (access(fullPath, X_OK) == 0) {
                free(pathCopy);
                return fullPath; 
            }
            free(fullPath);
            directory = strtok(NULL, ":");
        }
        free(pathCopy);
    }
    return NULL; 
}

// Helper function to record history
void addHistory(char *command) {
	// Shift history if we already have 3 commands
    if (historyCount < 3) {
        strcpy(history[historyCount], command);
        historyCount++;
    } else {
        strcpy(history[0], history[1]);
        strcpy(history[1], history[2]);
        strcpy(history[2], command);
    }
}