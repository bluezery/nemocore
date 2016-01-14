#ifndef	__NEMOUX_SHOW_HELPER_H__
#define	__NEMOUX_SHOW_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>
#include <nemotimer.h>
#include <nemocanvas.h>
#include <nemoegl.h>

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>
#include <taleevent.h>
#include <talegrab.h>
#include <talegesture.h>

#include <nemoshow.h>

struct showcontext {
	struct nemotool *tool;

	struct nemotimer *timer;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;

	int32_t width, height;
};

#define NEMOSHOW_AT(show, at)			(((struct showcontext *)nemoshow_get_context(show))->at)

extern struct nemoshow *nemoshow_create_canvas(struct nemotool *tool, int32_t width, int32_t height, nemotale_dispatch_event_t dispatch);
extern void nemoshow_destroy_canvas(struct nemoshow *show);

extern void nemoshow_destroy_canvas_on_idle(struct nemoshow *show);

extern void nemoshow_revoke_canvas(struct nemoshow *show);

extern void nemoshow_dispatch_frame(struct nemoshow *show);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
