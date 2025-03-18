#ifndef UTILS_H
#define UTILS_H

#define MAX_IDS 100

typedef struct {
	unsigned int ids[MAX_IDS];
	unsigned int size;
} IDSpace_t;

extern IDSpace_t id_space;

void init_id_space();
int obtain_id(unsigned int*);
int release_id(unsigned int);

#endif /* UTILS_H */
