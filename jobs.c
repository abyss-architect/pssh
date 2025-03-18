#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "jobs.h"
#include "utils.h"
#include "parse.h"

JobSystem_t job_system;

/* Initialize the free_job_i array with sequentially incrementing integers so
 * that the add_job function can figure out the
 **/
void init_job_system()
{
	init_id_space();
	job_system.njobs = 0;
}

Job *create_job(Parse *parse)
{
	Job *job = (Job *) malloc(sizeof(Job));
	job->name = (char *) malloc(sizeof(char) * strlen(parse->cmd));
	strcpy(job->name, parse->cmd);
	job->pids = (pid_t *) calloc(parse->ntasks, sizeof(pid_t));
	job->npids = parse->ntasks;

	return job;
}

void destroy_job(Job *job)
{
	free(job->name);
	free(job->pids);
	free(job);
}

int add_job(Job **job, Parse *parse)
{
	unsigned int id;
	if (obtain_id(&id) == -1) {
		perror("jobs.c: cannot obtain id\n");
		return -1;
	}

	Job *new_job = create_job(parse);

	new_job->id = id;

	job_system.jobs[id] = new_job;
	job_system.njobs++;

	*job = new_job;

	return 0;
}

int remove_job(Job *job)
{
	unsigned int id = job->id;
	if (release_id(id) == -1) {
		perror("jobs.c: cannot release id\n");
		return -1;
	}

	job_system.jobs[id] = NULL;
	job_system.njobs--;

	destroy_job(job);

	return 0;
}

int set_fg_pgrp(pid_t pgrp)
{
	void (*sav)(int sig);

	if (pgrp == 0)
		pgrp = getpgrp();

	if (!isatty(STDERR_FILENO)) {
		perror("jobs.c: cannot set fg pgrp since STDERR_FILENO is not a tty\n");
		return -1;
	}

	sav = signal(SIGTTOU, SIG_IGN);
	tcsetpgrp(STDERR_FILENO, pgrp);
	signal(SIGTTOU, sav);

	return 0;
}

void put_job_fg(Job *job)
{
	set_fg_pgrp(job->pgid);
	job->status = FG;
	job_system.fg_job = job;
}

int get_job_by_pid(Job **job, pid_t pid)
{
	unsigned int i, j;
	Job *curr_job;
	for (i = 0; i < job_system.njobs; i++) {
		curr_job = job_system.jobs[i];
		if (curr_job == NULL)
			continue;

		for (j = 0; j < curr_job->npids; j++) {
			if (curr_job->pids[j] == pid) {
				*job = curr_job;
				return 0;
			}
		}
	}

	return -1; /* Could not find a job containing pid */
}

char *job_status_to_str(JobStatus status)
{
	if (status == STOPPED)
		return "suspended";
	if (status == TERM)
		return "done";

	return "running";
}

void print_job(Job *job)
{
	unsigned int i;
	if (job == NULL)
		return;

	printf("+---------------+\n");
	printf("|  Print   Job  |\n");
	printf("+---------------+\n");
	printf("|name    : %s   |\n", job->name);
	printf("|id      : %d    |\n", job->id);
	printf("|status  : %s   |\n", job_status_to_str(job->status));
	printf("|pgid    : %d |\n", job->pgid);
	printf("|pids (%d):      |\n", job->npids);

	for (i = 0; i < job->npids; i++)
		printf("|  %d         |\n", job->pids[i]);

	printf("+---------------+\n");
}

void print_job_system()
{
	unsigned int i;
	for (i = 0; i < MAX_JOBS; i++)
		print_job(job_system.jobs[i]);
}
