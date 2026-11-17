#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]){
	FILE *f;

	if (argc < 2){
		fprintf(stderr, "using: %s <file>\n", argv[0]);
		return 1;
	}

	printf("before setuid:\n");
	printf("real uid: %d\n", getuid());
	printf("effective uid: %d\n", geteuid());

	f = fopen(argv[1], "r");
	if (f == NULL) {
	perror("fopen");
	} else {
		printf("file opens succesfully \n");
		fclose(f);
	}
	
	if (setuid(getuid()) == -1) perror ("setuid");
	
	printf ("after setuid: \n");
	printf("real uid: %d\n", getuid());
	printf("effective uid: %d\n", geteuid());
	
	f = fopen(argv[1], "r");
	if (f == NULL) {
		perror("fopen");
	} else {
		printf("file opens succesfully \n");
		fclose(f);
	}

	return 0;
}


