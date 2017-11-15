#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include "funcs.h"

#define VLOCK			"/etc/vusers/locked"
#define VPASSWD			"/etc/vusers/passwd"
#define VSHADOW			"/etc/vusers/shadow"
#define VMAIL			"vmail"

char *groups[] = {"users", "bus", "temp", "free", NULL};

#define MAINNAME_FIELD		0
#define EXISTS_FIELD		1
#define REALNAME_FIELD		2
#define ADDRESS_FIELD		3
#define CITY_FIELD		4
#define STATE_FIELD		5
#define ZIPCODE_FIELD		6
#define HOMEPHONE_FIELD		7
#define WORKPHONE_FIELD		8
#define REFERREDBY_FIELD	9
#define GROUP_FIELD		10
#define SECONDARY_FIELD		11
#define MAILONLY_FIELD		12
#define DOMAIN_FIELD		13
#define SEPARATE_FIELD		14
#define FORWARD_FIELD		15
#define ADDANOTHER_FIELD	16

#define DATA_WIDTH		128

typedef struct {
  char *prompt;
  char *data;
} InputField;

InputField Fields[] = {
	{"Main account name", NULL},
	{"Account exists.  Add to it? [Y/n]", NULL},
	{"Real name", NULL},
	{"Address", NULL},
	{"City", NULL},
	{"State", NULL},
	{"Zip code", NULL},
	{"Home phone", NULL},
	{"Work phone", NULL},
	{"Referred by", NULL},
	{"Group", NULL},
	{"Secondary Account Name", NULL},
	{"For mail only? [Y/n]", NULL},
	{"Domain", NULL},
	{"Separate mailbox? [y/N]", NULL},
	{"Forward to", NULL},
	{"Add another account? [Y/n]", NULL},
	{NULL, NULL}
};

#define MainName		Fields[MAINNAME_FIELD].data
#define Exists			Fields[EXISTS_FIELD].data
#define RealName		Fields[REALNAME_FIELD].data
#define Address			Fields[ADDRESS_FIELD].data
#define City			Fields[CITY_FIELD].data
#define State			Fields[STATE_FIELD].data
#define ZipCode			Fields[ZIPCODE_FIELD].data
#define HomePhone		Fields[HOMEPHONE_FIELD].data
#define WorkPhone		Fields[WORKPHONE_FIELD].data
#define ReferredBy		Fields[REFERREDBY_FIELD].data
#define Group			Fields[GROUP_FIELD].data
#define SecondaryUser		Fields[SECONDARY_FIELD].data
#define MailOnly		Fields[MAILONLY_FIELD].data
#define Domain 			Fields[DOMAIN_FIELD].data
#define SepMailBox		Fields[SEPARATE_FIELD].data
#define ForwardTo		Fields[FORWARD_FIELD].data
#define AddAnother		Fields[ADDANOTHER_FIELD].data

#define Prompt_For(index, d_width) \
  temp = Fields[index].data; \
  Fields[index].data = \
	Prompt(Fields[index].prompt, p_wid, Fields[index].data, d_width); \
  free(temp);

void Clear_All_Data(void)
{
  int i;

  for (i = 0; Fields[i].prompt; i++) {
    free(Fields[i].data);
    Fields[i++].data = NULL;
  }
}

int WidestPrompt (InputField *Fields)
{
  int widest, this, i;

  widest = 0;
  i = 0;
  for (i = 0; Fields[i].prompt; i++) {
    this = strlen(Fields[i].prompt)
		+ (Fields[i].data ? strlen(Fields[i].data) + 3 : 0);
    if (this > widest)
      widest = this;
  }
  return widest;
}

int mylookup (char *name)
{
  return -1;
}






void UserInterface (int p_wid, int (*lookup)(char *))
{
  char *temp;
  int i;
  int GroupId;
  int TempId;

  printf("\nWidth = %d\n\n",p_wid);

  Prompt_For(DOMAIN_FIELD, DATA_WIDTH);
  Prompt_For(MAINNAME_FIELD, DATA_WIDTH);
  if ((GroupId = lookup(MainName)) == 0) {
    Prompt_For(REALNAME_FIELD, DATA_WIDTH);
    Prompt_For(ADDRESS_FIELD, DATA_WIDTH);
    Prompt_For(CITY_FIELD, DATA_WIDTH);
    Prompt_For(STATE_FIELD, DATA_WIDTH);
    Prompt_For(ZIPCODE_FIELD, DATA_WIDTH);
    Prompt_For(HOMEPHONE_FIELD, DATA_WIDTH);
    Prompt_For(WORKPHONE_FIELD, DATA_WIDTH);
    Prompt_For(REFERREDBY_FIELD, DATA_WIDTH);
    printf("\nValid groups are [");
    for (i = 0; groups[i]; i++) {
      if (i) printf(" %s", groups[i]);
      else printf("%s", groups[i]);
    }
    printf("]\n\n");
    Group = malloc(strlen(groups[0]) + 1);
    strcpy(Group, groups[0]);
    Prompt_For(GROUP_FIELD, DATA_WIDTH);
  } else {
    TempId = 0;
    while (! TempId);
      Prompt_For(SECONDARY_FIELD, DATA_WIDTH);
      if (SecondaryUser) {
        if ((TempId = lookup(SecondaryUser)) > 0) {
          printf("\nUser already exists.\n\n");
        }
    }
    Prompt_For(MAILONLY_FIELD, DATA_WIDTH);
    if ((*MailOnly == 'Y') || (*MailOnly == 'y')) {
      Prompt_For(SEPARATE_FIELD, DATA_WIDTH);
      if ((*SepMailBox != 'Y') || (*SepMailBox == 'y')) {
        if ((! ForwardTo) || (! *ForwardTo)) {
          if (ForwardTo) free(ForwardTo);
          ForwardTo = malloc(strlen(MainName) + strlen(Domain) + 2);
          sprintf("%s@%s", MainName, Domain);
        }
        Prompt_For(FORWARD_FIELD, DATA_WIDTH);
      } 
    }
  


    Prompt_For(REALNAME_FIELD, DATA_WIDTH);




  }
}

void clean_exit (int val)
{
  printf("\n\nCleaning up....\n");
  fflush(stdout);
  DropLock(VLOCK);
  exit(5);
}

int main (int argc, char **argv)
{
  int Prompt_Width;

  if (SetLock(VLOCK) == -1) {
    printf("Virtual password file locked.\n");
    exit(10);
  }
  signal(SIGTERM, clean_exit);
  signal(SIGINT, clean_exit);


  Prompt_Width = WidestPrompt(Fields);
  UserInterface(Prompt_Width, mylookup);

  DropLock(VLOCK);
  return 0;
}
