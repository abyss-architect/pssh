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
	unsigned int ncomplete;
	JobStatus status;
} Job;

typedef struct {
	Job *jobs[MAX_JOBS];
	Job *fg_job;
	unsigned int njobs;
} JobSystem_t;

extern JobSystem_t job_system;

void init_job_system();
Job *create_job(unsigned int);
void destroy_job(Job*);
int add_job(Job**, unsigned int);
int remove_job(Job*);
int kill_job(Job*);
int set_fg_pgrp(pid_t);
void put_job_fg(Job*);
int get_job_by_pid(Job*, pid_t);

/* Debug */
char *job_status_to_str(JobStatus);
void print_job(Job*);
void print_job_system();

#endif /* JOBS_H */
