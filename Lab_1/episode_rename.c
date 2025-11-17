#include "utilities.h"

void main(int argc, char *argv[]){
	if (argc != 2){
		/* Number of inputs should be 2; otherwise aborts */
		fprintf(stderr, "Wrong number of inputs! Aborting...\n");
		exit(EXIT_FAILURE);
	}
	char *filename = argv[1]; /* argv[0] is the executable name */
	fprintf(stdout, "\"%s\"", filename);
	fprintf(stdout, " ");

	char *new_filename = updateFilename( filename );
	fprintf(stdout, "%s\n", new_filename);
	free( new_filename );

	return;
}
