#ifndef	__NEMOMOTZ_OBJECT_H__
#define	__NEMOMOTZ_OBJECT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_OBJECT_FILL_FLAG = (1 << 0),
	NEMOMOTZ_OBJECT_STROKE_FLAG = (1 << 1),
	NEMOMOTZ_OBJECT_TRANSFORM_FLAG = (1 << 2)
} NemoMotzObjectFlag;

typedef enum {
	NEMOMOTZ_OBJECT_SHAPE_DIRTY = (1 << 8),
	NEMOMOTZ_OBJECT_TRANSFORM_DIRTY = (1 << 9),
	NEMOMOTZ_OBJECT_COLOR_DIRTY = (1 << 10),
	NEMOMOTZ_OBJECT_STROKE_WIDTH_DIRTY = (1 << 11),
	NEMOMOTZ_OBJECT_FONT_DIRTY = (1 << 12),
	NEMOMOTZ_OBJECT_FONT_SIZE_DIRTY = (1 << 13),
	NEMOMOTZ_OBJECT_TEXT_DIRTY = (1 << 14)
} NemoMotzObjectDirty;

typedef enum {
	NEMOMOTZ_OBJECT_NONE_SHAPE = 0,
	NEMOMOTZ_OBJECT_LINE_SHAPE = 1,
	NEMOMOTZ_OBJECT_RECT_SHAPE = 2,
	NEMOMOTZ_OBJECT_ROUND_RECT_SHAPE = 3,
	NEMOMOTZ_OBJECT_CIRCLE_SHAPE = 4,
	NEMOMOTZ_OBJECT_ARC_SHAPE = 5,
	NEMOMOTZ_OBJECT_TEXT_SHAPE = 6,
	NEMOMOTZ_OBJECT_LAST_SHAPE
} NemoMotzObjectShape;

struct motzobject {
	struct motzone one;

	struct tozzstyle *style;
	struct tozzmatrix *matrix;
	struct tozzmatrix *inverse;

	int shape;

	float tx, ty;
	float sx, sy;
	float rz;

	float x, y;
	float w, h;
	float radius;

	float r, g, b, a;

	float stroke_width;

	char *font_path;
	int font_index;
	float font_size;
	char *text;
};

#define NEMOMOTZ_OBJECT(one)		((struct motzobject *)container_of(one, struct motzobject, one))

extern struct motzone *nemomotz_object_create(void);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, int, shape, shape, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, int, shape, shape);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(object, float, tx, tx, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, tx, tx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(object, float, ty, ty, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, ty, ty);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(object, float, sx, sx, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, sx, sx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(object, float, sy, sy, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, sy, sy);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(object, float, rz, rz, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, rz, rz);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, x, x, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, x, x);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, y, y, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, y, y);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, w, width, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, w, width);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, h, height, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, h, height);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, radius, radius, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, radius, radius);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, r, red, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, r, red);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, g, green, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, g, green);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, b, blue, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, b, blue);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, a, alpha, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, a, alpha);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, stroke_width, stroke_width, NEMOMOTZ_OBJECT_STROKE_WIDTH_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, stroke_width, stroke_width);

NEMOMOTZ_DECLARE_DUP_STRING(object, font_path, font_path, NEMOMOTZ_OBJECT_FONT_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, char *, font_path, font_path);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, int, font_index, font_index, NEMOMOTZ_OBJECT_FONT_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, int, font_index, font_index);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(object, float, font_size, font_size, NEMOMOTZ_OBJECT_FONT_SIZE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, float, font_size, font_size);
NEMOMOTZ_DECLARE_DUP_STRING(object, text, text, NEMOMOTZ_OBJECT_TEXT_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(object, char *, text, text);

NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(object, tx, tx, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(object, ty, ty, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(object, sx, sx, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(object, sy, sy, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(object, rz, rz, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY, NEMOMOTZ_OBJECT_TRANSFORM_FLAG);

NEMOMOTZ_DECLARE_SET_TRANSITION(object, x, x, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, y, y, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, w, width, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, h, height, NEMOMOTZ_OBJECT_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, radius, radius, NEMOMOTZ_OBJECT_SHAPE_DIRTY);

NEMOMOTZ_DECLARE_SET_TRANSITION(object, r, red, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, g, green, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, b, blue, NEMOMOTZ_OBJECT_COLOR_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(object, a, alpha, NEMOMOTZ_OBJECT_COLOR_DIRTY);

NEMOMOTZ_DECLARE_SET_TRANSITION(object, stroke_width, stroke_width, NEMOMOTZ_OBJECT_STROKE_WIDTH_DIRTY);

NEMOMOTZ_DECLARE_SET_TRANSITION(object, font_size, font_size, NEMOMOTZ_OBJECT_FONT_SIZE_DIRTY);

NEMOMOTZ_DECLARE_CHECK_TRANSITION_DESTROY(object);
NEMOMOTZ_DECLARE_CHECK_TRANSITION_REVOKE(object);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
