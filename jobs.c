#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "jobs.h"
#include "utils.h"

JobSystem_t job_system;

/* Initialize the free_job_i array with sequentially incrementing integers so
 * that the add_job function can figure out the
 **/
void init_job_system()
{
	init_id_space();
	job_system.njobs = 0;
}

Job *create_job(unsigned int npids)
{
	Job *job = (Job *) malloc(sizeof(Job));
	job->name = (char *) malloc(sizeof(char) * MAX_NAME_SIZE);
	job->pids = (pid_t *) calloc(npids, sizeof(pid_t));
	job->npids = npids;

	return job;
}

void destroy_job(Job *job)
{
	free(job->name);
	free(job->pids);
	free(job);
}

int add_job(Job **job, unsigned int npids)
{
	unsigned int id;
	if (obtain_id(&id) == -1) {
		perror("jobs.c: cannot obtain id\n");
		return -1;
	}

	Job *new_job = create_job(npids);

	new_job->id = id;
	sprintf(new_job->name, "%%%d", id);

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

char *job_status_to_str(JobStatus status)
{
	if (status == STOPPED)
		return "S ";
	if (status == TERM)
		return "T ";
	if (status == BG)
		return "BG";

	return "FG";
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
