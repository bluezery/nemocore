#ifndef __SKIA_HELPER_H__
#define __SKI_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int skia_get_text_width(const char *font, double fontsize, const char *text);
extern int skia_draw_text(void *pixels, int32_t width, int32_t height, const char *font, double fontsize, const char *text);
extern int skia_draw_image(void *pixels, int32_t width, int32_t height, const char *uri);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif