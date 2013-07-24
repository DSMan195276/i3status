
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>
#include <X11/Xlib.h>

#include "i3status.h"
#include "queue.h"

Display *display = NULL;



void print_window_title (yajl_gen json_gen, char *buffer, const char *fmt) {
	Window focus;
	char *window_name;
	int revert;
	char *outwalk = buffer;
	const char *walk;

	if (display == NULL) 
		display = XOpenDisplay(NULL);

	XGetInputFocus(display, &focus, &revert);
	XFetchName(display, focus, &window_name);

	for (walk = fmt; *walk != '\0'; walk++) {
		if ( *walk != '%' ) {
			*(outwalk++) = *walk;
			continue;
		}

		if (BEGINS_WITH(walk+1, "title")) {
			outwalk += sprintf(outwalk, "%s", window_name);
			walk += strlen("title");
		}
	}
	*outwalk = '\0';
	OUTPUT_FULL_TEXT(buffer);

	if (window_name)
		XFree(window_name);

	return ;
}
