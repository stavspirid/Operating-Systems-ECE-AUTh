#include "utilities.h"
char *updateFilename( char *filename ){
	char tmp1[20], tmp2[20];
	int season, episode;
	char *result = (char *)malloc(8 * sizeof(char));
	
	if (sscanf(filename, "%s %d %s %d", tmp1, &season, tmp2, &episode) != 4)
        	return NULL;
	// sscanf( filename, "%s %d %s %d", tmp1, &season, tmp2, &episode );
	sprintf( result, "S%02d_E%02d", season, episode );

	return result;
}
