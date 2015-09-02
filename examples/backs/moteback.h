#ifndef	__MOTEBACK_H__
#define	__MOTEBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemomote.h>

struct moteback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	struct nemomote mote;
	struct moteblast blast;
	struct nemozone zone;

	double secs;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
