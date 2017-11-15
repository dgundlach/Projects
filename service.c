#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>

static char **environ;

int main(int argc, char **argv) {

	int rv;
	int rc;
	char servicePath[NAME_MAX + 16] = "/etc/init.d/";
	char *service;

	if (argc == 3) {
		service = servicePath + strlen(servicePath);
		sprintf(service, "%s", argv[1]);
		argv[1] = servicePath;
		rv = execve(servicePath, &argv[1], environ);
		rc = errno;
		switch(rc) {
			case EACCES:
				printf("%s: Access denied.\n", service);
				break;
			case ENOEXEC:
				printf("%s: Not executable.\n", service);
				break;
			case ENOENT:
				printf("%s: Not a service.\n", service);
				break;
			default:
				printf("General error.\n");
		}
		exit(rv);
	}
	printf("Usage: %s <service> <arg>", basename(argv[0]));
}
