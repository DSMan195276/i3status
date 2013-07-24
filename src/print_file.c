
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include "i3status.h"
#include "queue.h"

void print_file(yajl_gen json_gen, char *buffer, const char *fmt, const char *file, bool display_if_empty) {
	char *outwalk = buffer;
	const char *walk;
	FILE *infile;
	size_t men_size = 100;
	char *line = (char*)calloc(men_size, 1);

	infile = fopen(file, "r");
	if (infile) {
		getline(&line, &men_size, infile);
		fclose(infile);
	}
	if (line[0] != '\0' || display_if_empty) {
		SEC_OPEN_MAP("file");
	        for (walk = fmt; *walk != '\0'; walk++) {
	
			if ( *walk != '%' ) {
				*(outwalk++) = *walk;
				continue;
			}

			if (BEGINS_WITH(walk+1, "file")) {
				outwalk += sprintf(outwalk, "%s", line);
				walk += strlen("file");
			}
		}
		*outwalk = '\0';
		OUTPUT_FULL_TEXT(buffer);
		SEC_CLOSE_MAP;
	}


	if (line)
		free(line);

	return ;
}
