#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "builtin.h"
#include "parse.h"
#include "jobs.h"

static char *builtin[] = {
    "exit",   /* exits the shell */
    "which",  /* displays full path to command */
	"jobs",
    NULL
};


int is_builtin(char *cmd)
{
    int i;

    for (i=0; builtin[i]; i++) {
        if (!strcmp(cmd, builtin[i]))
            return 1;
    }

    return 0;
}

void which(Task T) {
	char *path;
	char *_path;
	char *path_dir;
	char program_path[PATH_MAX];

	if (T.argv[1] == NULL) {
		printf("pssh: builtin command: missing a program to search for\n");
		return;
	}

	if (is_builtin(T.argv[1])) {
		printf("%s: shell built-in command\n", T.argv[1]);
		return;
	}

	// If a relative or absolute path was provided, then check if it exists
	if (access(T.argv[1], X_OK) == 0) {
		printf("%s\n", T.argv[1]);
		return;
	}

	// Otherwise, check the system path for the program
	path = getenv("PATH");
	if (path == NULL) {
		printf("pssh: builtin command: PATH variable not found\n");
		return;
	}
 	
	// Duplicate the path so that strtok doesn't override the contents of the PATH env
	_path = strdup(path);
	path_dir = strtok(_path, ":");
	while (path_dir != NULL) {
		strcpy(program_path, path_dir);
		strcat(program_path, "/");
		strcat(program_path, T.argv[1]);
		if (access(program_path, X_OK) == 0) {
			printf("%s\n", program_path);
			break;
		}

		path_dir = strtok(NULL, ":");
	}

	free(_path);
}

void jobs()
{
	unsigned int i;
	Job *job;
	for (i = 0; i < MAX_JOBS; i++) {
		job = job_system.jobs[i];
		if (job == NULL)
			continue;

		printf("[%d] + %s\t%s\n", job->id, job_status_to_str(job->status), job->name);
	}
}

void builtin_execute(Task T)
{
    if (!strcmp(T.cmd, "exit")) {
        exit (EXIT_SUCCESS);
    } else if (!strcmp(T.cmd, "which")) {
		which(T);
    } else if (!strcmp(T.cmd, "jobs")) {
		jobs();
	} else {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
