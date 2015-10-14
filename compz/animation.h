#ifndef	__NEMO_ANIMATION_H__
#define	__NEMO_ANIMATION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoease.h>
#include <nemolist.h>

struct nemoanimation;

typedef void (*nemoanimation_frame_t)(struct nemoanimation *animation, double progress);
typedef void (*nemoanimation_done_t)(struct nemoanimation *animation);

struct nemoanimation {
	struct wl_list link;

	uint32_t frame_count;

	uint32_t stime, etime;
	uint32_t delay;
	uint32_t duration;

	struct nemoease ease;

	nemoanimation_frame_t frame;
	nemoanimation_done_t done;

	void *userdata;
};

extern struct nemoanimation *nemoanimation_create(uint32_t ease, uint32_t delay, uint32_t duration);
extern void nemoanimation_destroy(struct nemoanimation *animation);

static inline void nemoanimation_set_dispatch_frame(struct nemoanimation *animation, nemoanimation_frame_t dispatch)
{
	animation->frame = dispatch;
}

static inline void nemoanimation_set_dispatch_done(struct nemoanimation *animation, nemoanimation_done_t dispatch)
{
	animation->done = dispatch;
}

static inline void nemoanimation_set_userdata(struct nemoanimation *animation, void *data)
{
	animation->userdata = data;
}

static inline void *nemoanimation_get_userdata(struct nemoanimation *animation)
{
	return animation->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
