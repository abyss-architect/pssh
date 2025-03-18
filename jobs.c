#include <string.h>
#include <stdio.h>

#include "jobs.h"
#include "utils.h"

/* Initialize the free_job_i array with sequentially incrementing integers so
 * that the add_job function can figure out the
 **/
void init_job_system()
{
	init_id_space();
	job_system.njobs = 0;
}

int add_job(Job *job)
{
	unsigned int *id;
	if (obtain_id(id) == -1) {
		perror("jobs.c: cannot obtain id\n");
		return -1;
	}
	
	job->id = *id;
	sprintf(job->name, "%%%d", next_job_n);

	job_system.jobs[*id] = *job;
	job_system.njobs++;

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

	return 0;
}

void print_job_system()
{
	unsigned int i, j;
	Job job;
	for (i = 0; i < MAX_JOBS; i++) {
		job = job_system.jobs[i];
		printf("+-----------+");
		printf("name   : %s\n", job.name);
		printf("id     : %d\n", job.id);

		printf("pgid   : %d\n", job.pgid);
		printf("pids (%d):\n", job.npids);

		for (j = 0; j < job.npids; j++)
			printf("%d  ", job.pids[j]);

		printf("\n");
	}
}
