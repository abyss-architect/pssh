#ifndef JOBS_H
#define JOBS_H

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

#endif /* JOBS_H */
