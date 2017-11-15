#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
extern int errno;
extern char *crypt();
extern char *malloc();
extern char **environ;

char up[513];
int uplen;

int main(int argc,char **argv)
{
	struct passwd *pw;
	struct group *gr;
	char *login;
	char *password;
	char *stored;
	char *encrypted;
	int r;
	int i;
	char **newenv;
	int numenv;
	FILE *p;

	if (!argv[1]) _exit(2);
	pw = getpwuid(getuid());
	if (!pw) _exit(1); /* If I don't exist, we're in big trouble */
	if (chdir(pw->pw_dir) == -1) _exit(111);

	uplen = 0;
	for (;;) {
		r = read(3,up + uplen,sizeof(up) - uplen);
		while ((r == -1) && (errno == EINTR));
		if (r == -1) _exit(111);
		if (r == 0) break;
		uplen += r;
		if (uplen >= sizeof(up)) _exit(1);
	}

	close(3);

	i = 0;
	login = up + i;
	while (up[i++]) if (i == uplen) _exit(2);
	password = up + i;
	if (i == uplen) _exit(2);
	while (up[i++]) if (i == uplen) _exit(2);

	p = fopen("passwd", "r");
	if (!p) _exit(1);
	while (gr = fgetgrent(p)) if (!strcmp(login, gr->gr_name)) break;
	fclose(p);
	if (!gr) _exit(1);

	stored = gr->gr_passwd;

	encrypted = crypt(password,stored);

	for (i = 0;i < sizeof(up);++i) up[i] = 0;

	if (!*stored || strcmp(encrypted,stored)) _exit(1);

	setenv("USER", pw->pw_name, 1);
	setenv("HOME", pw->pw_dir, 1);
	setenv("HOST", gr->gr_name, 1);
	setenv("DOMAIN", gr->gr_mem[0], 1);

	execvp(argv[1],argv + 1);
	_exit(111);
}
