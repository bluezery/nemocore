#ifndef	__NEMOFX_GL_SWEEP_H__
#define	__NEMOFX_GL_SWEEP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

typedef enum {
	NEMOFX_GLSWEEP_SIMPLE_TYPE = 0,
	NEMOFX_GLSWEEP_HORIZONTAL_TYPE = 1,
	NEMOFX_GLSWEEP_VERTICAL_TYPE = 2,
	NEMOFX_GLSWEEP_CIRCLE_TYPE = 3,
	NEMOFX_GLSWEEP_RECT_TYPE = 4,
	NEMOFX_GLSWEEP_FAN_TYPE = 5,
	NEMOFX_GLSWEEP_MASK_TYPE = 6,
	NEMOFX_GLSWEEP_LAST_TYPE
} NemoFXGLSweepType;

extern struct glsweep *nemofx_glsweep_create(int32_t width, int32_t height);
extern void nemofx_glsweep_destroy(struct glsweep *sweep);

extern void nemofx_glsweep_use_fbo(struct glsweep *sweep);

extern void nemofx_glsweep_ref_snapshot(struct glsweep *sweep, uint32_t texture, int32_t width, int32_t height);
extern void nemofx_glsweep_set_snapshot(struct glsweep *sweep, uint32_t texture, int32_t width, int32_t height);
extern void nemofx_glsweep_put_snapshot(struct glsweep *sweep);

extern void nemofx_glsweep_set_timing(struct glsweep *sweep, float t);
extern void nemofx_glsweep_set_rotate(struct glsweep *sweep, float r);
extern void nemofx_glsweep_set_type(struct glsweep *sweep, int type);
extern void nemofx_glsweep_set_point(struct glsweep *sweep, float x, float y);
extern void nemofx_glsweep_set_mask(struct glsweep *sweep, uint32_t mask);

extern void nemofx_glsweep_resize(struct glsweep *sweep, int32_t width, int32_t height);
extern uint32_t nemofx_glsweep_dispatch(struct glsweep *sweep, uint32_t texture);

extern uint32_t nemofx_glsweep_get_texture(struct glsweep *sweep);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
