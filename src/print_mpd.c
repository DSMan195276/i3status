
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>
#include <mpd/message.h>

#include "i3status.h"
#include "queue.h"

struct mpd_connection *mpd_conn = NULL;

void print_mpd(yajl_gen json_gen, char *buffer, const char *host, const int port, const char *pass) {
	struct mpd_song *song;
	const char *artist = NULL, *title = NULL, *file = NULL;
	char *outwalk;

	if (mpd_conn == NULL) {
		if ((mpd_conn = mpd_connection_new(host, port, 0)) != NULL) {
			if (mpd_connection_get_error(mpd_conn) != MPD_ERROR_SUCCESS) {
				mpd_connection_free(mpd_conn);
				mpd_conn = NULL;
			} else if (pass != NULL) {
				mpd_send_password(mpd_conn, pass);
				if (mpd_connection_get_error(mpd_conn) != MPD_ERROR_SUCCESS) {
					mpd_connection_free(mpd_conn);
					mpd_conn = NULL;
				}
			}
		}
	}
	
	if (mpd_conn != NULL) {
		song = mpd_run_current_song(mpd_conn);
		if (mpd_connection_get_error(mpd_conn) != MPD_ERROR_SUCCESS) {
			mpd_connection_free(mpd_conn);
			mpd_conn = NULL;
		} else if (song != NULL) {
			artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
			title  = mpd_song_get_tag(song, MPD_TAG_TITLE,  0);
			file   = mpd_song_get_tag(song, MPD_TAG_NAME,   0);
		}
		SEC_OPEN_MAP("mpd");
		buffer[0] = '\0';
		strcat(buffer, "MPD: ");
		if (title != NULL) {
			strcat(buffer, title);
			if (artist != NULL) {
				strcat(buffer, " - ");
				strcat(buffer, artist);
			}
		} else {
			if (file != NULL) {
				strcat(buffer, file);
			} else {
				strcat(buffer, "Playing");
			}
		}
		outwalk = buffer + strlen(buffer);
		OUTPUT_FULL_TEXT(buffer);
		SEC_CLOSE_MAP;
	}



	if (song) 
		mpd_song_free(song);
	return ;
}
