#ifndef JOBS_H
#define JOBS_H

#include <unistd.h>
#include "utils.h"

#define MAX_JOBS 100
#define MAX_NAME_SIZE 8

typedef enum {
	STOPPED,
	TERM,
	BG,
	FG
} JobStatus;

typedef struct {
	char* name;
	unsigned int id;
	pid_t pgid;
	pid_t* pids;
	unsigned int npids;
	JobStatus status;
} Job;

typedef struct {
	Job *jobs[MAX_JOBS];
	unsigned int njobs;
} JobSystem_t;

extern JobSystem_t job_system;

void init_job_system();
Job *create_job(unsigned int);
void destroy_job(Job*);
int add_job(Job**, unsigned int);
int remove_job(Job*);
int kill_job(Job*);

/* Debug */
void print_job(Job*);
void print_job_system();

#endif /* JOBS_H */
