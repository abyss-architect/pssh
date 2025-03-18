#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

Job *create_job()
{
	Job *job = malloc(sizeof(Job));
	job->name = malloc(sizeof(char) * MAX_NAME_SIZE);

	return job;
}

void destroy_job(Job *job)
{
	free(job->name);
	free(job);
}

int add_job(Job **job)
{
	unsigned int id;
	if (obtain_id(&id) == -1) {
		perror("jobs.c: cannot obtain id\n");
		return -1;
	}

	Job *new_job = create_job();

	new_job->id = id;
	sprintf(new_job->name, "%%%d", id);

	job_system.jobs[id] = new_job;
	job_system.njobs++;

	*job = new_job;

	return 0;
}

int remove_job(Job *job)
{
	printf("ID: %d\n", job->id);
	unsigned int id = job->id;
	printf("RELEASING\n");
	if (release_id(id) == -1) {
		perror("jobs.c: cannot release id\n");
		return -1;
	}

	printf("NULLING\n");
	job_system.jobs[id] = NULL;
	job_system.njobs--;

	printf("DESTROYING\n");
	destroy_job(job);

	return 0;
}

void print_job(Job *job)
{
	unsigned int i;
	if (job == NULL)
		return;

	printf("+------------+\n");
	printf("| Print  Job |\n");
	printf("+------------+\n");
	printf("|name    : %s|\n", job->name);
	printf("|id      : %d |\n", job->id);
	printf("|pgid    : %d |\n", job->pgid);
	printf("|pids (%d):   |\n", job->npids);

	for (i = 0; i < job->npids; i++)
		printf("|  %d  |\n", job->pids[i]);

	printf("+------------+\n");
}

void print_job_system()
{
	unsigned int i;
	for (i = 0; i < MAX_JOBS; i++)
		print_job(job_system.jobs[i]);
}
