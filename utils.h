#ifndef UTILS_H
#define UTILS_H

#define MAX_IDS 100

typedef struct {
	unsigned int ids[MAX_IDS];
	unsigned int size;
} IDSpace_t;

void init_id_space(IDSpace_t*);
int obtain_id(IDSpace_t*, unsigned int*);
int release_id(IDSpace_t*, unsigned int);

#endif /* UTILS_H */
