#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/speedscale.h>

int nemomote_speedscale_update(struct nemomote *mote, double secs, double x, double y, double z)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		NEMOMOTE_VELOCITY_X(mote, i) *= x;
		NEMOMOTE_VELOCITY_Y(mote, i) *= y;
		NEMOMOTE_VELOCITY_Z(mote, i) *= z;
	}

	return 0;
}
