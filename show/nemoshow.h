#ifndef	__NEMOSHOW_H__
#define	__NEMOSHOW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>
#include <nemolist.h>
#include <nemolistener.h>

#include <showone.h>
#include <showscene.h>
#include <showcanvas.h>
#include <showitem.h>
#include <showcolor.h>
#include <showsequence.h>
#include <showease.h>
#include <showtransition.h>
#include <showmatrix.h>
#include <showpath.h>
#include <showfilter.h>
#include <showshader.h>
#include <showexpr.h>
#include <showfont.h>
#include <showeasy.h>
#include <showpoly.h>
#include <showpipe.h>
#include <svghelper.h>

#include <nemotale.h>
#include <nemolist.h>
#include <nemolistener.h>

struct nemoshow;

typedef void (*nemoshow_dispatch_transition_done_t)(void *userdata);
typedef void (*nemoshow_dispatch_event_t)(struct nemoshow *show, void *event);
typedef void (*nemoshow_dispatch_transform_t)(struct nemoshow *show, int32_t visible);
typedef void (*nemoshow_dispatch_fullscreen_t)(struct nemoshow *show, int32_t active, int32_t opaque);
typedef void (*nemoshow_dispatch_layer_t)(struct nemoshow *show, int32_t visible);
typedef void (*nemoshow_dispatch_destroy_t)(struct nemoshow *show);

struct nemoshow {
	struct nemotale *tale;

	struct showexpr *expr;
	struct showsymtable *stable;

	struct showone *scene;
	struct nemolistener scene_destroy_listener;

	uint32_t width, height;
	double sx, sy;

	struct nemolist one_list;
	struct nemolist dirty_list;
	struct nemolist bounds_list;
	struct nemolist canvas_list;
	struct nemolist transition_list;
	struct nemolist transition_destroy_list;

	nemoshow_dispatch_transition_done_t dispatch_done;
	void *dispatch_data;

	nemoshow_dispatch_event_t dispatch_event;
	nemoshow_dispatch_transform_t dispatch_transform;
	nemoshow_dispatch_fullscreen_t dispatch_fullscreen;
	nemoshow_dispatch_layer_t dispatch_layer;
	nemoshow_dispatch_destroy_t dispatch_destroy;

	uint32_t dirty_serial;
	uint32_t transition_serial;

	void *context;
	void *userdata;
};

#define nemoshow_for_each(one, show)	\
	nemolist_for_each(one, &((show)->one_list), link)
#define nemoshow_for_each_reverse(one, show)	\
	nemolist_for_each_reverse(one, &((show)->one_list), link)

extern void nemoshow_initialize(void);
extern void nemoshow_finalize(void);

extern struct nemoshow *nemoshow_create(void);
extern void nemoshow_destroy(struct nemoshow *show);

extern struct showone *nemoshow_search_one(struct nemoshow *show, const char *id);

#ifdef NEMOUX_WITH_SHOWEXPR
extern void nemoshow_update_symbol(struct nemoshow *show, const char *name, double value);
extern void nemoshow_update_expression(struct nemoshow *show);
extern void nemoshow_update_one_expression(struct nemoshow *show, struct showone *one);
extern void nemoshow_update_one_expression_without_dirty(struct nemoshow *show, struct showone *one);
#endif

extern void nemoshow_render_one(struct nemoshow *show);

extern int nemoshow_set_scene(struct nemoshow *show, struct showone *one);
extern void nemoshow_put_scene(struct nemoshow *show);

extern int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height);
extern int nemoshow_set_scale(struct nemoshow *show, double sx, double sy);

extern void nemoshow_attach_canvas(struct nemoshow *show, struct showone *one);
extern void nemoshow_detach_canvas(struct nemoshow *show, struct showone *one);

extern void nemoshow_above_canvas(struct nemoshow *show, struct showone *one, struct showone *above);
extern void nemoshow_below_canvas(struct nemoshow *show, struct showone *one, struct showone *below);

extern int nemoshow_contain_canvas(struct nemoshow *show, struct showone *one, float x, float y, float *sx, float *sy);

extern void nemoshow_damage_canvas_all(struct nemoshow *show);

extern void nemoshow_attach_one(struct nemoshow *show, struct showone *one);
extern void nemoshow_detach_one(struct showone *one);

extern void nemoshow_attach_ones(struct nemoshow *show, struct showone *one);
extern void nemoshow_detach_ones(struct showone *one);

extern void nemoshow_attach_transition(struct nemoshow *show, struct showtransition *trans);
extern void nemoshow_detach_transition(struct nemoshow *show, struct showtransition *trans);
extern void nemoshow_attach_transition_after(struct nemoshow *show, struct showtransition *trans, struct showtransition *ntrans);
extern void nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs);
extern void nemoshow_destroy_transition(struct nemoshow *show);
extern int nemoshow_has_transition(struct nemoshow *show);

extern void nemoshow_dump_all(struct nemoshow *show, FILE *out);

static inline void nemoshow_set_tale(struct nemoshow *show, struct nemotale *tale)
{
	show->tale = tale;
}

static inline struct nemotale *nemoshow_get_tale(struct nemoshow *show)
{
	return show->tale;
}

static inline void nemoshow_set_context(struct nemoshow *show, void *context)
{
	show->context = context;
}

static inline void *nemoshow_get_context(struct nemoshow *show)
{
	return show->context;
}

static inline void nemoshow_set_userdata(struct nemoshow *show, void *data)
{
	show->userdata = data;
}

static inline void *nemoshow_get_userdata(struct nemoshow *show)
{
	return show->userdata;
}

static inline void nemoshow_set_dispatch_transition_done(struct nemoshow *show, nemoshow_dispatch_transition_done_t dispatch, void *data)
{
	show->dispatch_done = dispatch;
	show->dispatch_data = data;
}

static inline void nemoshow_set_dispatch_event(struct nemoshow *show, nemoshow_dispatch_event_t dispatch)
{
	if (dispatch == NULL) {
		nemotale_put_state(show->tale, NEMOTALE_NOFOCUS_STATE);
	} else {
		nemotale_set_state(show->tale, NEMOTALE_NOFOCUS_STATE);
	}

	show->dispatch_event = dispatch;
}

static inline void nemoshow_set_dispatch_transform(struct nemoshow *show, nemoshow_dispatch_transform_t dispatch)
{
	show->dispatch_transform = dispatch;
}

static inline void nemoshow_set_dispatch_fullscreen(struct nemoshow *show, nemoshow_dispatch_fullscreen_t dispatch)
{
	show->dispatch_fullscreen = dispatch;
}

static inline void nemoshow_set_dispatch_layer(struct nemoshow *show, nemoshow_dispatch_layer_t dispatch)
{
	show->dispatch_layer = dispatch;
}

static inline void nemoshow_set_dispatch_destroy(struct nemoshow *show, nemoshow_dispatch_destroy_t dispatch)
{
	show->dispatch_destroy = dispatch;
}

static inline void nemoshow_set_close_width(struct nemoshow *show, uint32_t width)
{
	nemotale_set_close_width(show->tale, width);
}

static inline void nemoshow_set_close_height(struct nemoshow *show, uint32_t height)
{
	nemotale_set_close_height(show->tale, height);
}

static inline void nemoshow_set_minimum_width(struct nemoshow *show, uint32_t width)
{
	nemotale_set_minimum_width(show->tale, width);
}

static inline void nemoshow_set_minimum_height(struct nemoshow *show, uint32_t height)
{
	nemotale_set_minimum_height(show->tale, height);
}

static inline void nemoshow_set_maximum_width(struct nemoshow *show, uint32_t width)
{
	nemotale_set_maximum_width(show->tale, width);
}

static inline void nemoshow_set_maximum_height(struct nemoshow *show, uint32_t height)
{
	nemotale_set_maximum_height(show->tale, height);
}

static inline uint32_t nemoshow_get_close_width(struct nemoshow *show)
{
	return nemotale_get_close_width(show->tale);
}

static inline uint32_t nemoshow_get_close_height(struct nemoshow *show)
{
	return nemotale_get_close_height(show->tale);
}

static inline uint32_t nemoshow_get_minimum_width(struct nemoshow *show)
{
	return nemotale_get_minimum_width(show->tale);
}

static inline uint32_t nemoshow_get_minimum_height(struct nemoshow *show)
{
	return nemotale_get_minimum_height(show->tale);
}

static inline uint32_t nemoshow_get_maximum_width(struct nemoshow *show)
{
	return nemotale_get_maximum_width(show->tale);
}

static inline uint32_t nemoshow_get_maximum_height(struct nemoshow *show)
{
	return nemotale_get_maximum_height(show->tale);
}

#include <showgrab.h>
#include <showevent.h>

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
