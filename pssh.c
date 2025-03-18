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
#include "jobs.h"

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

pid_t execute_task(Task T, pid_t pgid)
{
	pid_t pid;
	if ((pid = vfork()) == -1) {
		perror("pssh: failed to vfork\n");
		return -1;
	}

	setpgid(pid, pgid);
	
	if (pid == 0) {
		// Child process
		execvp(T.cmd, T.argv);
	}

	return pid;
}

void wait_for_children(Job *job)
{
	unsigned int t;

#if DEBUG_PARSE
	printf("Waiting for children (%d)...\n", job->npids);
#endif

	for (t = 0; t < job->npids; t++) {
		waitpid(job->pids[t], NULL, 0);
		job->ncomplete++;
	}

	remove_job(job);

#if DEBUG_PARSE
	printf("Done waiting (%d)...\n", job->npids);
#endif

	set_fg_pgrp(0); /* Take the foreground back */
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

pid_t _execute_task(Task T, pid_t pgid)
{
	pid_t child_pid;
	if (is_builtin(T.cmd)) {
		builtin_execute(T);
	}
	else if (command_found(T.cmd)) {
		child_pid = execute_task(T, pgid);
		
		if (child_pid == -1) {
			printf("pssh: found but can't exec: %s\n", T.cmd);
		}
	}
	else {
		printf("pssh: command not found: %s\n", T.cmd);
	}

	return child_pid;
}

/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks(Parse *P)
{
	Job *job;
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

	add_job(&job, P);
	job->status = P->background ? BG : FG;

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

			job->pids[t] = _execute_task(P->tasks[t], job->pids[0]);
			
			if (swap_fd(STDIN_FILENO, fd[READ_SIDE]) == -1) {
				printf("pssh: failed to swap the file descriptors\n");
				return;
			}
		} else {
			job->pids[t] = _execute_task(P->tasks[t], job->pids[0]);
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

	job->pgid = job->pids[0];

	if (job->status == FG) {
		print_job(job);
		put_job_fg(job);
		wait_for_children(job);
	}
	if (job->status == BG) {
		printf("[%d]", job->id);

		for (t = 0; t < job->npids; t++)
			printf(" %d", job->pids[t]);

		printf("\n");

		print_job(job);
	}

}


int main(int argc, char **argv)
{
	char *prompt;
	char *cmdline;
	Parse *P;

	init_job_system();

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
