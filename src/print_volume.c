// vim:ts=8:expandtab
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#ifdef LINUX
#include <alsa/asoundlib.h>
#include <alloca.h>
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#endif

#ifdef __OpenBSD__
#include <fcntl.h>
#include <unistd.h>
#include <soundcard.h>
#endif

#include "i3status.h"
#include "queue.h"

void print_volume(yajl_gen json_gen, char *buffer, const char *fmt, const char *device, const char *mixer, int mixer_idx, const char *mpc) {
        char *outwalk = buffer;
	int pbval = 1;

        /* Printing volume only works with ALSA at the moment */
        if (output_format == O_I3BAR) {
                char *instance;
                asprintf(&instance, "%s.%s.%d", device, mixer, mixer_idx);
                INSTANCE(instance);
                free(instance);
        }
#ifdef LINUX
	int err;
	snd_mixer_t *m;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
	long min, max, val;
	int avg;
        unsigned int output_len = 0, pid = 0;
        char *mpc_output = NULL, mpc_cmd[2014], pid_line[1024];
        FILE *mpc_prog;
        ssize_t char_len = 0;
        FILE *pianobar_cmd;

        int pand_flag = 0;

	if ((err = snd_mixer_open(&m, 0)) < 0) {
		fprintf(stderr, "i3status: ALSA: Cannot open mixer: %s\n", snd_strerror(err));
		goto out;
	}

	/* Attach this mixer handle to the given device */
	if ((err = snd_mixer_attach(m, device)) < 0) {
		fprintf(stderr, "i3status: ALSA: Cannot attach mixer to device: %s\n", snd_strerror(err));
		snd_mixer_close(m);
		goto out;
	}

	/* Register this mixer */
	if ((err = snd_mixer_selem_register(m, NULL, NULL)) < 0) {
		fprintf(stderr, "i3status: ALSA: snd_mixer_selem_register: %s\n", snd_strerror(err));
		snd_mixer_close(m);
		goto out;
	}

	if ((err = snd_mixer_load(m)) < 0) {
		fprintf(stderr, "i3status: ALSA: snd_mixer_load: %s\n", snd_strerror(err));
		snd_mixer_close(m);
		goto out;
	}

	snd_mixer_selem_id_malloc(&sid);
	if (sid == NULL) {
		snd_mixer_close(m);
		goto out;
	}

	/* Find the given mixer */
	snd_mixer_selem_id_set_index(sid, mixer_idx);
	snd_mixer_selem_id_set_name(sid, mixer);
	if (!(elem = snd_mixer_find_selem(m, sid))) {
		fprintf(stderr, "i3status: ALSA: Cannot find mixer %s (index %i)\n",
			snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		snd_mixer_close(m);
		snd_mixer_selem_id_free(sid);
		goto out;
	}

	/* Get the volume range to convert the volume later */
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

	snd_mixer_handle_events (m);
	snd_mixer_selem_get_playback_volume (elem, 0, &val);
	if (max != 100) {
		float avgf = ((float)val / max) * 100;
		avg = (int)avgf;
		avg = (avgf - avg < 0.5 ? avg : (avg+1));
	} else avg = (int)val;

	/* Check for mute */
	if (snd_mixer_selem_has_playback_switch(elem)) {
		if ((err = snd_mixer_selem_get_playback_switch(elem, 0, &pbval)) < 0)
			fprintf (stderr, "i3status: ALSA: playback_switch: %s\n", snd_strerror(err));
		if (!pbval)  {
			START_COLOR("color_degraded");
			avg = 0;
		}
	}

	snd_mixer_close(m);
	snd_mixer_selem_id_free(sid);
        
        if (mpc[0] != '\0') {
                mpc_cmd[0] = '\0';
                strcat (mpc_cmd, "mpc -f \"");
                strcat (mpc_cmd, mpc);
                strcat (mpc_cmd, "\" 2>&1");
                mpc_prog = popen(mpc_cmd, "r");
                if (mpc_prog != NULL) {
                        char_len = getline(&mpc_output, &output_len, mpc_prog);
                        if (char_len > 0) 
                                mpc_output[char_len-1] = '\0';
                        
                        pclose(mpc_prog);
                        if ((strncmp(mpc_output, "error: Connection refused", sizeof("error: Connection refused") - 1))==0) {
                                
                              mpc_output[0] = '\0'; 
                        }
                }
                if (mpc_output==NULL||mpc_output[0]=='\0') {
                        pianobar_cmd = popen("pidof pianobarfly", "r");
                        fgets(pid_line, sizeof(pid_line), pianobar_cmd);
                        pid = strtoul(pid_line, NULL, 10);
                        pclose(pianobar_cmd);
                        if (pid > 0) {
                                pianobar_cmd = fopen("/tmp/pandora-playing.txt", "r");
                                if (pianobar_cmd!=NULL) {
                                        char_len = getline(&mpc_output, &output_len, pianobar_cmd);
                                        if (char_len > 0)
                                                mpc_output[char_len-1] = '\0';
                                        pand_flag = 1;
                                } 
                                fclose(pianobar_cmd);
                        }
                }
        }

	const char *walk = fmt;
	for (; *walk != '\0'; walk++) {
		if (*walk != '%') {
                        *(outwalk++) = *walk;
			continue;
		}
		if (BEGINS_WITH(walk+1, "volume")) {
			outwalk += sprintf(outwalk, "%d%%", avg);
			walk += strlen("volume");
		}
                if (BEGINS_WITH(walk+1, "mpc")) {
                        if (mpc_output[0] != '\0') {
                                if (!pand_flag) {
                                        outwalk += sprintf(outwalk, " MPD: %s", mpc_output);
                                } else {
                                        outwalk += sprintf(outwalk, " Pandora: %s", mpc_output);
                                }
                        }
                        walk += strlen("mpc");
                }
	}
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
        char *mixerpath;
        char defaultmixer[] = "/dev/mixer";
        int mixfd, vol, devmask = 0;
        pbval = 1;

        if (mixer_idx > 0)
                asprintf(&mixerpath, "/dev/mixer%d", mixer_idx);
        else
                mixerpath = defaultmixer;

        if ((mixfd = open(mixerpath, O_RDWR)) < 0)
                return;

        if (mixer_idx > 0)
                free(mixerpath);

        if (ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
                return;
        if (ioctl(mixfd, MIXER_READ(0),&vol) == -1)
                return;

        if (((vol & 0x7f) == 0) && (((vol >> 8) & 0x7f) == 0)) {
                START_COLOR("color_degraded");
                pbval = 0;
        }

        const char *walk = fmt;
        for (; *walk != '\0'; walk++) {
                if (*walk != '%') {
                        *(outwalk++) = *walk;
                        continue;
                }
                if (BEGINS_WITH(walk+1, "volume")) {
                        outwalk += sprintf(outwalk, "%d%%", vol & 0x7f);
                        walk += strlen("volume");
                }
        }
        close(mixfd);
#endif

out:
        *outwalk = '\0';
        OUTPUT_FULL_TEXT(buffer);

        if (!pbval)
		END_COLOR;

        if (mpc_output!=NULL) 
                free (mpc_output);
}
