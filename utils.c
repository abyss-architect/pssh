#include <stdlib.h>

#include "utils.h"

void init_id_space(IDSpace_t *id_space)
{
	unsigned int i;

	id_space->size = MAX_IDS;

	for (i = 0; i < id_space->size; i++)
		id_space->ids[i] = i;
}

int obtain_id(IDSpace_t *id_space, unsigned int *id)
{
	unsigned int i;

	for (i = 0; i < id_space->size; i++) {
		*id = id_space->ids[i];
		if (*id < id_space->size) {
			id_space->ids[i] = id_space->size; /* Mark the ID as in-use */
			return 0;
		}
	}

	return -1; /* All the IDs have been taken */
}

int release_id(IDSpace_t *id_space, unsigned int id)
{
	/* Not in the range of IDs */
	if (id > id_space->size)
		return -1;

	id_space->ids[id] = id;
}
