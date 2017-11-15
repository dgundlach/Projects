#include <stdio.h>

typedef struct uuid_t {
	int device;
	char uuid[37];
} uuid_t;

int main(int argc, char **argv) {

	char x[4096];
	uuid_t *uuid = (void *)x;
	void *y = uuid;

	uuid->device=100;
	sprintf(uuid->uuid, "This is a test.");

// Address of a member

	printf ("%x\n", &uuid->device);
	printf ("%x\n\n", &uuid->uuid);

// Address of a cast

	printf ("%x\n", &((uuid_t*)y)->device);
	printf ("%x\n\n", &((uuid_t*)y)->uuid);

// Value of a cast

	printf ("%d\n", ((uuid_t*)y)->device);
	printf ("%s\n", ((uuid_t*)y)->uuid);
}
