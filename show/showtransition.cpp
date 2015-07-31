#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showtransition.h>
#include <showsequence.h>
#include <nemobox.h>
#include <nemomisc.h>

struct showtransition *nemoshow_transition_create(struct showone *ease, uint32_t duration, uint32_t delay)
{
	struct showtransition *trans;

	trans = (struct showtransition *)malloc(sizeof(struct showtransition));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct showtransition));

	trans->sequences = (struct showone **)malloc(sizeof(struct showone *) * 4);
	trans->nsequences = 0;
	trans->ssequences = 4;

	trans->ease = NEMOSHOW_EASE(ease);

	trans->duration = duration;
	trans->delay = delay;

	return trans;
}

void nemoshow_transition_destroy(struct showtransition *trans)
{
	free(trans->sequences);
	free(trans);
}

void nemoshow_transition_attach_sequence(struct showtransition *trans, struct showone *sequence)
{
	NEMOBOX_APPEND(trans->sequences, trans->ssequences, trans->nsequences, sequence);

	nemoshow_sequence_prepare(sequence);
}

int nemoshow_transition_update(struct showtransition *trans, uint32_t time)
{
	double t;
	int done = 0;
	int i;

	if (trans->stime == 0) {
		trans->stime = time + trans->delay;
		trans->etime = time + trans->delay + trans->duration;
	}

	if (trans->stime > time)
		return 0;

	if (trans->etime <= time) {
		t = 1.0f;
		done = 1;
	} else {
		t = nemoease_get(&trans->ease->ease, time - trans->stime, trans->duration);
	}

	for (i = 0; i < trans->nsequences; i++) {
		nemoshow_sequence_dispatch(trans->sequences[i], t);
	}

	return done;
}
