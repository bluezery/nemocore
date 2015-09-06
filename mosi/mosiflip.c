#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomosi.h>
#include <mosiflip.h>
#include <nemomisc.h>

void nemomosi_flip_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t s, uint32_t d0, uint32_t d1)
{
	struct mosione *one;
	int i, j, n = 0;

	for (j = 0; j < mosi->width; j++, n++) {
		for (i = 0; i < mosi->height; i++) {
			one = &mosi->ones[i * mosi->width + j];

			one->stime = msecs + s * n;
			one->etime = one->stime + random_get_int(d0, d1);

			one->has_transition = 1;
		}
	}
}
