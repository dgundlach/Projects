#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define STR			"ata"
#define PER_LINE	6
#define FIRST_ITEM	1
#define LAST_ITEM	256

int main (int argc, char **argv) {

	char *tablename = NULL;
	char *str		= STR;
	int first_item	= FIRST_ITEM;
	int last_item	= LAST_ITEM;
	int per_line	= PER_LINE;
	int usenull		= 0;
	int i, j;
	int c;
	char lc;

	while (1) {
		if ((c = getopt(argc, argv, "f:l:np:s:?h")) == -1) break;

		switch (c) {
			case 'f':
				first_item = strtol(optarg, NULL, 10);
				break;
			case 'l':
				last_item = strtol(optarg, NULL, 10);
				break;
			case 'n':
				usenull = 1;
				break;
			case 'p':
				per_line = strtol(optarg, NULL, 10);
				break;
			case 's':
				str = optarg;
				break;
			case 'h':
			case '?':
			default:
				printf("Usage: %s [-f <first>] [-l <last>] [-n]"
					" [-p <per line>] [-s <string>] [name]\n",
					basename(argv[0]));
				exit(-1);
		}
	}
	if (optind < argc) {
		tablename = argv[optind++];
	}
	if (usenull) {
		last_item++;
	}
	if (tablename) {
		printf("char *%s[%d] = {\n", tablename,
				last_item - first_item + 1);
	}
	i = first_item;
	while (i <= last_item) {
		lc = '\t';
		for (j=0; j<per_line; j++) {
			if (i > last_item) break;
			if ((i == last_item) && usenull) {
				printf("%cNULL", lc);
			} else {
				printf("%c\"%s%d\"", lc, str, i);

			}
			if (i != last_item) {
				printf(",");
				if (j != (per_line - 1)) {
					if (i < 10) printf("  ");
					else if (i < 100) printf(" ");
				}
			}
			lc = ' ';
			i++;
		}
		printf("\n");
	}
	if (tablename) {
		printf("};\n");
	}
}
