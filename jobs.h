#ifndef JOBS_H
#define JOBS_H

#include <unistd.h>

#define MAX_JOBS 100

typedef enum {
	STOPPED,
	TERM,
	BG,
	FG
} JobStatus;

typedef struct {
	char* name;
	pid_t* pids;
	unsigned int npids;
	pid_t pgid;
	JobStatus status;
} Job;

typedef struct {
	Job job;
	JobNode* next;
} JobNode;

typedef struct {
	Job jobs[MAX_JOBS];
	unsigned int free_job_i[MAX_JOBS];
	unsigned int njobs;
} JobSystem;

void init_job_system(JobSystem*);
int add_job(JobSystem*, Job*);
int remove_job(JobSystem*, Job*);
int kill_job(Job*);

/* Helpers */
unsigned int get_next_job_n(JobSystem*);

#endif /* JOBS_H */
