#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <shadow.h>
#include <string.h>
extern int errno;
extern char *crypt();
extern char *malloc();
extern char **environ;
char *md5_crypt(char *, char *);


char up[513];
int uplen;

#define DEFAULTDOMAIN	"/var/qmail/control/defaultdomain"
#define VIRTUALHOSTS	"/var/qmail/control/virtualdomains"

int main(int argc,char **argv)
{
	struct passwd *pw;
	struct spwd *sp;
	char *login;
	char *password;
	char *stored;
	char *encrypted;
        char *c;
        char *domain = NULL;
        char buffer[128];
	char *virtual;
	char *maintainer;
	char *maildir;
	int r;
	int i;
	FILE *p;
	struct stat st;
	uid_t uid = 0;
	gid_t gid = 0;

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

	c = login;
	while (*c) {
		if ((*c == ':') || (*c == '@')) {
			*c++ = '\0';
			domain = c;
			break;
		}
		c++;
	}
	if (!domain) {
		if (!stat(DEFAULTDOMAIN,&st)) {
			domain = malloc(st.st_size + 1);
                	if ((p = fopen(DEFAULTDOMAIN, "r"))) {
				fgets(domain,st.st_size,p);
				i = 0;
				while (domain[i]) {
					if ((domain[i] == '\r') || (domain[i] == '\n')) {
						domain[i] = '\0';
						break;
					}
					i++;
				}
				fclose(p);
			} else {
				domain = malloc(256);
				gethostname(domain, 256);
			}
		} else {
			domain = malloc(256);
			gethostname(domain, 256);
		}
	}
	if ((p = fopen(VIRTUALHOSTS, "r"))) {
		while (fgets(buffer, sizeof(buffer), p)) {
			virtual = buffer;
			i = 0;
			while (buffer[i] && (buffer[i] != ':')) i++;
			if (!buffer[i]) exit(2);
                        buffer[i++] = '\0';
			maintainer = buffer + i;
			while (buffer[i] && (buffer[i] != '\r')
					 && (buffer[i] != '\n')) i++;
			buffer[i] = '\0';
			if (!strcmp(virtual,domain)) {
				if (!(pw = getpwnam(maintainer))) exit(2);
				if (chdir(pw->pw_dir) == -1) exit(2);
				if (chdir(domain) == -1) exit(2);
				uid = pw->pw_uid;
				gid = pw->pw_gid;
                                setenv("USER", pw->pw_name, 1);
				setenv("HOME", pw->pw_dir, 1);
				if (!(p = fopen("passwd", "r"))) exit(1);
				while ((pw = fgetpwent(p)))
					if (!strcmp(login,pw->pw_name)) break;
				fclose(p);
				if (!pw) exit(1);
				stored = pw->pw_passwd;
				maildir = malloc(strlen(pw->pw_dir) + 3);
				sprintf(maildir,".%s",pw->pw_dir);
				argv[argc - 1] = maildir;
				break;
			}
		}
		fclose(p);
	}
	if (!uid) {
		chdir("/etc");
		if (!(p = fopen("passwd", "r"))) exit(1);
		while ((pw = fgetpwent(p)))
			if (!strcmp(login, pw->pw_name)) break;
		fclose(p);
		if (!pw) exit(1);
		if (!(p = fopen("shadow", "r"))) exit(1);
                while ((sp = fgetspent(p)))
			if (!strcmp(login, sp->sp_namp)) break;
		fclose(p);
		if (!sp) exit(1);
		setenv("USER", pw->pw_name, 1);
		setenv("HOME", pw->pw_dir, 1);
                stored = sp->sp_pwdp;
		uid = pw->pw_uid;
		gid = pw->pw_gid;
		if (chdir(pw->pw_dir) == -1) exit(1);
	}
	if (!strncmp(stored,"$1$",3)) {
		encrypted = md5_crypt(password, stored);
	} else {
		encrypted = crypt(password, stored);
	}

	for (i = 0;i < sizeof(up);++i) up[i] = 0;
	if (!*stored || strcmp(encrypted,stored)) _exit(1);

	setgroups(1,&gid);
	setgid(gid);
	setuid(uid);

	execvp(argv[1],argv + 1);
	_exit(111);
}
