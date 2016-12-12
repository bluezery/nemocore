#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <playqueue.h>
#include <nemomisc.h>

struct playqueue *nemoplay_queue_create(void)
{
	struct playqueue *queue;

	queue = (struct playqueue *)malloc(sizeof(struct playqueue));
	if (queue == NULL)
		return NULL;
	memset(queue, 0, sizeof(struct playqueue));

	if (pthread_mutex_init(&queue->lock, NULL) != 0)
		goto err1;

	if (pthread_cond_init(&queue->signal, NULL) != 0)
		goto err2;

	nemolist_init(&queue->list);

	return queue;

err2:
	pthread_mutex_destroy(&queue->lock);

err1:
	free(queue);

	return NULL;
}

void nemoplay_queue_destroy(struct playqueue *queue)
{
	struct playone *one, *none;

	pthread_mutex_lock(&queue->lock);

	nemolist_for_each_safe(one, none, &queue->list, link)
		nemoplay_one_destroy(one);

	nemolist_remove(&queue->list);

	pthread_mutex_unlock(&queue->lock);

	pthread_cond_destroy(&queue->signal);
	pthread_mutex_destroy(&queue->lock);

	free(queue);
}

void nemoplay_queue_enqueue(struct playqueue *queue, struct playone *one)
{
	pthread_mutex_lock(&queue->lock);

	nemolist_enqueue(&queue->list, &one->link);
	queue->count++;

	pthread_cond_signal(&queue->signal);

	pthread_mutex_unlock(&queue->lock);
}

void nemoplay_queue_enqueue_tail(struct playqueue *queue, struct playone *one)
{
	pthread_mutex_lock(&queue->lock);

	nemolist_enqueue_tail(&queue->list, &one->link);
	queue->count++;

	pthread_cond_signal(&queue->signal);

	pthread_mutex_unlock(&queue->lock);
}

struct playone *nemoplay_queue_dequeue(struct playqueue *queue)
{
	struct nemolist *elm;

	pthread_mutex_lock(&queue->lock);

	elm = nemolist_dequeue(&queue->list);
	if (elm != NULL)
		queue->count--;

	pthread_mutex_unlock(&queue->lock);

	return elm != NULL ? container_of(elm, struct playone, link) : NULL;
}

struct playone *nemoplay_queue_peek(struct playqueue *queue)
{
	struct playone *one = NULL;
	struct nemolist *elm;

	pthread_mutex_lock(&queue->lock);

	elm = nemolist_peek_tail(&queue->list);
	if (elm != NULL) {
		one = container_of(elm, struct playone, link);

		if (one->serial != queue->serial)
			one = NULL;
	}

	pthread_mutex_unlock(&queue->lock);

	return one;
}

int nemoplay_queue_peek_pts(struct playqueue *queue, double *pts)
{
	struct playone *one = NULL;
	struct nemolist *elm;

	pthread_mutex_lock(&queue->lock);

	elm = nemolist_peek_tail(&queue->list);
	if (elm != NULL) {
		one = container_of(elm, struct playone, link);

		if (one->serial != queue->serial)
			one = NULL;

		if (one != NULL)
			*pts = one->pts;
	}

	pthread_mutex_unlock(&queue->lock);

	return one != NULL;
}

void nemoplay_queue_wait(struct playqueue *queue)
{
	pthread_mutex_lock(&queue->lock);

	if (queue->state != NEMOPLAY_QUEUE_DONE_STATE)
		pthread_cond_wait(&queue->signal, &queue->lock);

	pthread_mutex_unlock(&queue->lock);
}

void nemoplay_queue_flush(struct playqueue *queue)
{
	struct playone *one, *none;

	pthread_mutex_lock(&queue->lock);

	nemolist_for_each_safe(one, none, &queue->list, link)
		nemoplay_one_destroy(one);

	queue->count = 0;

	queue->serial++;

	pthread_mutex_unlock(&queue->lock);
}

void nemoplay_queue_set_state(struct playqueue *queue, int state)
{
	pthread_mutex_lock(&queue->lock);

	queue->state = state;

	pthread_cond_signal(&queue->signal);

	pthread_mutex_unlock(&queue->lock);
}

struct playone *nemoplay_one_create(void)
{
	struct playone *one;

	one = (struct playone *)malloc(sizeof(struct playone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct playone));

	nemolist_init(&one->link);

	return one;
}

void nemoplay_one_destroy(struct playone *one)
{
	nemolist_remove(&one->link);

	if (one->data[0] != NULL)
		free(one->data[0]);
	if (one->data[1] != NULL)
		free(one->data[1]);
	if (one->data[2] != NULL)
		free(one->data[2]);
	if (one->data[3] != NULL)
		free(one->data[3]);

	free(one);
}
