#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <limits.h>

#include "builtin.h"
#include "parse.h"

/*******************************************
 * Set to 1 to view the command line parse *
 * Set to 0 before submitting!             *
 *******************************************/
#define DEBUG_PARSE 0

#define WRITE_SIDE 1
#define READ_SIDE 0

void print_banner()
{
	printf ("                    ________   \n");
	printf ("_________________________  /_  \n");
	printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
	printf ("__  /_/ /(__  )_(__  )_  / / / \n");
	printf ("_  .___//____/ /____/ /_/ /_/  \n");
	printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}

/* **returns** a string used to build the prompt
 * (DO NOT JUST printf() IN HERE!)
 *
 * Note:
 *   If you modify this function to return a string on the heap,
 *   be sure to free() it later when appropirate!  */
static char *build_prompt()
{
	char *command_line;
	char *prompt = "$ ";
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("pssh: couldn't get the current working directory\n");
		exit(EXIT_FAILURE);	
	}

	command_line = malloc(strlen(cwd) + strlen(prompt) + 1);
	strcpy(command_line, cwd);
	strcat(command_line, prompt);
	
	return command_line;
}

/* return true if command is found, either:
 *   - a valid fully qualified path was supplied to an existing file
 *   - the executable file was found in the system's PATH
 * false is returned otherwise */
static int command_found(const char *cmd)
{
	char *dir;
	char *tmp;
	char *PATH;
	char *state;
	char probe[PATH_MAX];

	int ret = 0;

	if (access(cmd, X_OK) == 0)
		return 1;

	PATH = strdup(getenv("PATH"));

	for (tmp=PATH; ; tmp=NULL) {
		dir = strtok_r(tmp, ":", &state);
		if (!dir)
			break;

		strncpy(probe, dir, PATH_MAX-1);
		strncat(probe, "/", PATH_MAX-1);
		strncat(probe, cmd, PATH_MAX-1);

		if (access(probe, X_OK) == 0) {
			ret = 1;
			break;
		}
	}

	free(PATH);
	return ret;
}

pid_t execute_task(Task T)
{
	pid_t pid;
	if ((pid = vfork()) == -1) {
		perror("pssh: failed to vfork\n");
		return -1;
	}
	
	if (pid == 0) {
		// Child process
		execvp(T.cmd, T.argv);
	}

	return pid;
}

void wait_for_children(int ntasks)
{
	unsigned int t;

#if DEBUG_PARSE
	printf("Waiting for children (%d)...\n", ntasks);
#endif

	for (t = 0; t < ntasks; t++) {
		wait(NULL);
	}

#if DEBUG_PARSE
	printf("Done waiting (%d)...\n", ntasks);
#endif
}

int swap_fd(int target, int source)
{
	int result;
	result = dup2(source, target);

	if (result == -1) {
		perror("pssh: failed to dup2\n");
		return -1;
	}

	if (close(source) == -1) {
		perror("pssh: failed to close file descriptor");
		return -1;
	}

	return result;
}

void _execute_task(Task T)
{
	if (is_builtin(T.cmd)) {
		builtin_execute(T);
	}
	else if (command_found(T.cmd)) {
		pid_t child_pid = execute_task(T);
		
		if (child_pid == -1) {
			printf("pssh: found but can't exec: %s\n", T.cmd);
		}
	}
	else {
		printf("pssh: command not found: %s\n", T.cmd);
	}
}

/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks(Parse *P)
{
	unsigned int t;
	int orig_fd[2];
	int fd[2];

	// Save the original file descriptors
	orig_fd[READ_SIDE] = dup(STDIN_FILENO);
	if (orig_fd[READ_SIDE] == -1) {
		perror("pssh: failed to dup\n");
		return;
	}

	orig_fd[WRITE_SIDE] = dup(STDOUT_FILENO);
	if (orig_fd[WRITE_SIDE] == -1) {
		perror("pssh: failed to dup\n");
		return;
	}

	for (t = 0; t < P->ntasks; t++) {
		// If this is the first task and there is an infile, set up the file descriptor table
		if (t == 0 && P->infile) {

#if DEBUG_PARSE
			printf("There was infile\n");
#endif

			fd[READ_SIDE] = open(P->infile, O_RDONLY);
			if (fd[READ_SIDE] == -1) {
				perror("pssh: couldn't open infile\n");
				return;
			}

			if (swap_fd(STDIN_FILENO, fd[READ_SIDE]) == -1) {
				printf("pssh: failed to swap the file descriptors\n");
				return;
			}
		}

		// If this is the last task and there is an outfile, set up the file descriptor table
		if (t == (P->ntasks - 1)) {
			if (P->outfile) {
#if DEBUG_PARSE
				printf("There was outfile\n");
#endif

				fd[WRITE_SIDE] = open(P->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fd[WRITE_SIDE] == -1) {
					perror("pssh: couldn't open outfile\n");
					return;
				}

				if (swap_fd(STDOUT_FILENO, fd[WRITE_SIDE]) == -1) {
					printf("pssh: failed to swap the file descriptors\n");
					return;
				}
			} else {
				// Reset the STDOUT_FILENO of the last task to original STDOUT
				if (dup2(orig_fd[WRITE_SIDE], STDOUT_FILENO) == -1) {
					perror("pssh: failed to dup2\n");
					return;
				}
			}
		}

		// If there are more tasks after this one, then pipe
		if ((t + 1) < P->ntasks) {
#if DEBUG_PARSE
			printf("There are multiple tasks\n");
#endif

			if (pipe(fd) == -1) {
				perror("pssh: failed to pipe\n");
				return;
			}

			if (swap_fd(STDOUT_FILENO, fd[WRITE_SIDE]) == -1) {
				printf("pssh: failed to swap the file descriptors\n");
				return;
			}

			_execute_task(P->tasks[t]);
			
			if (swap_fd(STDIN_FILENO, fd[READ_SIDE]) == -1) {
				printf("pssh: failed to swap the file descriptors\n");
				return;
			}
		} else {
			_execute_task(P->tasks[t]);
		}
	}

	// Reset the shell file descriptors back to their originals
	if (swap_fd(STDIN_FILENO, orig_fd[READ_SIDE]) == -1) {
		printf("pssh: failed to swap the file descriptors\n");
		return;
	}

	if (swap_fd(STDOUT_FILENO, orig_fd[WRITE_SIDE]) == -1) {
		printf("pssh: failed to swap the file descriptors\n");
		return;
	}

	wait_for_children(P->ntasks);
}


int main(int argc, char **argv)
{
	char *prompt;
	char *cmdline;
	Parse *P;

	print_banner();

	while (1) {
		/* do NOT replace readline() with scanf() or anything else! */
		prompt = build_prompt();
		cmdline = readline(prompt);
		if (!cmdline)	   /* EOF (ex: ctrl-d) */
			exit(EXIT_SUCCESS);

		P = parse_cmdline(cmdline);
		if (!P)
			goto next;

		if (P->invalid_syntax) {
			printf("pssh: invalid syntax\n");
			goto next;
		}

#if DEBUG_PARSE
		parse_debug(P);
#endif

		execute_tasks(P);

	next:
		parse_destroy(&P);
		free(prompt);
		free(cmdline);
	}
}
