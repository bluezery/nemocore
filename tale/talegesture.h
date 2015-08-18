#ifndef	__NEMOTALE_GESTURE_H__
#define	__NEMOTALE_GESTURE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotale.h>
#include <talenode.h>
#include <taleevent.h>

static inline int nemotale_pointer_is_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = nemotale_pointer_get_tap(tale, event->device);

	if (tap != NULL && event->time - tap->grab_time < 150) {
		return 1;
	}

	return 0;
}

static inline int nemotale_touch_is_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = nemotale_touch_get_tap(tale, event->device);

	if (tap != NULL && event->time - tap->grab_time < 150 && sqrtf(event->dx * event->dx + event->dy * event->dy) < 30) {
		return 1;
	}

	return 0;
}

static inline int nemotale_is_single_click(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	if (type & NEMOTALE_POINTER_UP_EVENT)
		return nemotale_pointer_is_single_click(tale, event);
	else if (type & NEMOTALE_TOUCH_UP_EVENT)
		return nemotale_touch_is_single_click(tale, event);

	return 0;
}

static inline int nemotale_is_down_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_DOWN_EVENT;
}

static inline int nemotale_is_motion_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_MOTION_EVENT;
}

static inline int nemotale_is_up_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_UP_EVENT;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
