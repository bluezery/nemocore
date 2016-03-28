#ifndef __NEMOPLAY_QUEUE_H__
#define __NEMOPLAY_QUEUE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pthread.h>

#include <nemolist.h>

struct playone {
	struct nemolist link;

	int cmd;
};

struct playqueue {
	pthread_mutex_t lock;
	pthread_cond_t signal;

	struct nemolist list;
};

extern struct playqueue *nemoplay_queue_create(void);
extern void nemoplay_queue_destroy(struct playqueue *queue);

extern struct playone *nemoplay_queue_create_one(void);
extern void nemoplay_queue_destroy_one(struct playone *one);

extern void nemoplay_queue_enqueue(struct playqueue *queue, struct playone *one);
extern struct playone *nemoplay_queue_dequeue(struct playqueue *queue);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif