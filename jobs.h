#ifndef JOBS_H
#define JOBS_H

#include <unistd.h>
#include "utils.h"

#define MAX_JOBS 100

typedef enum {
	STOPPED,
	TERM,
	BG,
	FG
} JobStatus;

typedef struct {
	char* name;
	unsigned int id;
	pid_t* pids;
	unsigned int npids;
	pid_t pgid;
	JobStatus status;
} Job;

typedef struct {
	Job jobs[MAX_JOBS];
	unsigned int njobs;
} JobSystem_t;

extern JobSystem_t job_system;

void init_job_system();
int add_job(Job*);
int remove_job(Job*);
int kill_job(Job*);

/* Debug */
void print_job_system();

#endif /* JOBS_H */
