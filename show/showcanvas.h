#ifndef	__NEMOSHOW_CANVAS_H__
#define	__NEMOSHOW_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>

#include <showone.h>

#define	NEMOSHOW_CANVAS_TYPE_MAX		(32)

typedef enum {
	NEMOSHOW_CANVAS_NONE_TYPE = 0,
	NEMOSHOW_CANVAS_VECTOR_TYPE = 1,
	NEMOSHOW_CANVAS_OPENGL_TYPE = 2,
	NEMOSHOW_CANVAS_PIXMAN_TYPE = 3,
	NEMOSHOW_CANVAS_BACK_TYPE = 4,
	NEMOSHOW_CANVAS_LAST_TYPE
} NemoShowCanvasType;

struct nemoshow;

typedef void (*nemoshow_canvas_dispatch_render_t)(struct nemoshow *show, struct showone *one);

struct showcanvas {
	struct showone base;

	char type[NEMOSHOW_CANVAS_TYPE_MAX];
	uint32_t event;

	uint32_t fill;
	double fills[4];

	double alpha;

	struct talenode *node;

	struct nemotale *tale;
	struct talefbo *fbo;

	double width, height;

	struct {
		double width, height;

		double sx, sy;
	} viewport;

	double tx, ty;
	double ro;
	double px, py;

	int needs_redraw;
	int needs_full_redraw;

	nemoshow_canvas_dispatch_render_t dispatch_render;

	void *cc;
};

#define NEMOSHOW_CANVAS(one)					((struct showcanvas *)container_of(one, struct showcanvas, base))
#define NEMOSHOW_CANVAS_AT(one, at)		(NEMOSHOW_CANVAS(one)->at)

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern int nemoshow_canvas_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_canvas_update(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_set_type(struct showone *one, int type);
extern void nemoshow_canvas_set_event(struct showone *one, uint32_t event);
extern void nemoshow_canvas_set_alpha(struct showone *one, double alpha);

extern int nemoshow_canvas_attach_pixman(struct showone *one, void *data, int32_t width, int32_t height);
extern void nemoshow_canvas_detach_pixman(struct showone *one);
extern int nemoshow_canvas_resize_pixman(struct showone *one, int32_t width, int32_t height);

extern void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_set_viewport(struct nemoshow *show, struct showone *one, double sx, double sy);

extern void nemoshow_canvas_damage_region(struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoshow_canvas_damage_one(struct showone *one, struct showone *child);
extern void nemoshow_canvas_damage_all(struct showone *one);

extern void nemoshow_canvas_translate(struct showone *one, float tx, float ty);
extern void nemoshow_canvas_rotate(struct showone *one, float ro);
extern void nemoshow_canvas_pivot(struct showone *one, float px, float py);

extern struct showone *nemoshow_canvas_pick_one(struct showone *one, int x, int y);

static inline void nemoshow_canvas_needs_redraw(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->needs_redraw = 1;
}

static inline void nemoshow_canvas_set_dispatch_render(struct showone *one, nemoshow_canvas_dispatch_render_t dispatch_render)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_render) = dispatch_render;
}

static inline struct talenode *nemoshow_canvas_get_node(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return canvas->node;
}

static inline pixman_image_t *nemoshow_canvas_get_pixman(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_get_pixman(canvas->node);
}

static inline double nemoshow_canvas_get_viewport_sx(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, viewport.sx);
}

static inline double nemoshow_canvas_get_viewport_sy(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, viewport.sy);
}

static inline void nemoshow_canvas_set_width(struct showone *one, double width)
{
	NEMOSHOW_CANVAS_AT(one, width) = width;
}

static inline void nemoshow_canvas_set_height(struct showone *one, double height)
{
	NEMOSHOW_CANVAS_AT(one, height) = height;
}

static inline void nemoshow_canvas_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->fills[2] = r;
	canvas->fills[1] = g;
	canvas->fills[0] = b;
	canvas->fills[3] = a;

	canvas->fill = 1;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
