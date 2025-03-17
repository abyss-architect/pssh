#include <string.h>

#include "jobs.h"

/* Initialize the free_job_i array with sequentially incrementing integers so
 * that the add_job function can figure out the
 **/
void init_job_system(JobSystem *job_system)
{
	unsigned int i;

	for (i = 0; i < MAX_JOBS; i++)
		job_system->free_job_i[i] = i;

	job_system->njobs = 0;
}

int add_job(JobSystem *job_system, Job *job)
{
	unsigned int next_job_n = job_system->free_job_i[job_system->njobs];

	/* Cannot add any more jobs */
	if (next_job_n == MAX_JOBS - 1)
		return -1;

	sprintf(job->name, "%%%d", next_job_n);
	job_system->jobs[next_job_n] = job;

	return 0;
}

int remove_job(JobSystem *job_system, Job *job)
{

}
