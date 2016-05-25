#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoegl.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemomisc.h>

static void nemoshow_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_close_width(tale) || height < nemotale_get_close_height(tale)) {
		if (nemocanvas_dispatch_destroy(canvas) > 0)
			return;
	}

	if (show->dispatch_resize != NULL) {
		show->dispatch_resize(show, width, height);
		return;
	}

	nemotool_resize_egl_canvas(scon->eglcanvas, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_render_one(show);

	nemotale_composite_egl_full(scon->tale);
}

static void nemoshow_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (secs == 0 && nsecs == 0) {
		nemocanvas_dispatch_feedback(canvas);
	} else if (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, secs * 1000 + nsecs / 1000000);
		nemoshow_destroy_transition(show);

		nemocanvas_dispatch_feedback(canvas);
	}

	nemoshow_render_one(show);

	nemotale_composite_egl(tale, NULL);
}

static void nemoshow_dispatch_canvas_discard(struct nemocanvas *canvas)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocanvas_dispatch_frame(scon->canvas);
}

static int nemoshow_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);

	if (type & NEMOTOOL_POINTER_ENTER_EVENT) {
		nemotale_push_pointer_enter_event(tale, event->serial, event->device, event->x, event->y);
	} else if (type & NEMOTOOL_POINTER_LEAVE_EVENT) {
		nemotale_push_pointer_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_POINTER_MOTION_EVENT) {
		nemotale_push_pointer_motion_event(tale, event->serial, event->device, event->time, event->x, event->y);
	} else if (type & NEMOTOOL_POINTER_BUTTON_EVENT) {
		if (event->state == WL_POINTER_BUTTON_STATE_PRESSED)
			nemotale_push_pointer_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_pointer_up_event(tale, event->serial, event->device, event->time, event->value);
	} else if (type & NEMOTOOL_POINTER_AXIS_EVENT) {
		nemotale_push_pointer_axis_event(tale, event->serial, event->device, event->time, event->state, event->r);
	} else if (type & NEMOTOOL_KEYBOARD_ENTER_EVENT) {
		nemotale_push_keyboard_enter_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_KEYBOARD_LEAVE_EVENT) {
		nemotale_push_keyboard_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_KEYBOARD_KEY_EVENT) {
		if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
			nemotale_push_keyboard_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_keyboard_up_event(tale, event->serial, event->device, event->time, event->value);
	} else if (type & NEMOTOOL_KEYBOARD_MODIFIERS_EVENT) {
	} else if (type & NEMOTOOL_KEYBOARD_LAYOUT_EVENT) {
		nemotale_push_keyboard_layout_event(tale, event->serial, event->device, event->name);
	} else if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		nemotale_push_touch_down_event(tale, event->serial, event->device, event->time, event->x, event->y, event->gx, event->gy);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		nemotale_push_touch_up_event(tale, event->serial, event->device, event->time);
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		nemotale_push_touch_motion_event(tale, event->serial, event->device, event->time, event->x, event->y, event->gx, event->gy);
	} else if (type & NEMOTOOL_STICK_ENTER_EVENT) {
		nemotale_push_stick_enter_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_STICK_LEAVE_EVENT) {
		nemotale_push_stick_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_STICK_TRANSLATE_EVENT) {
		nemotale_push_stick_translate_event(tale, event->serial, event->device, event->time, event->x, event->y, event->z);
	} else if (type & NEMOTOOL_STICK_ROTATE_EVENT) {
		nemotale_push_stick_rotate_event(tale, event->serial, event->device, event->time, event->x, event->y, event->z);
	} else if (type & NEMOTOOL_STICK_BUTTON_EVENT) {
		if (event->state == WL_POINTER_BUTTON_STATE_PRESSED)
			nemotale_push_stick_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_stick_up_event(tale, event->serial, event->device, event->time, event->value);
	}

	return 0;
}

static void nemoshow_dispatch_canvas_transform(struct nemocanvas *canvas, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_transform != NULL)
		show->dispatch_transform(show, visible, x, y, width, height);
}

static void nemoshow_dispatch_canvas_layer(struct nemocanvas *canvas, int32_t visible)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_layer != NULL)
		show->dispatch_layer(show, visible);
}

static void nemoshow_dispatch_canvas_fullscreen(struct nemocanvas *canvas, int32_t id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_fullscreen != NULL)
		show->dispatch_fullscreen(show, id, x, y, width, height);
}

static int nemoshow_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_destroy != NULL)
		return show->dispatch_destroy(show);

	nemotool_exit(canvas->tool);

	return 1;
}

static void nemoshow_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct showcontext *scon = (struct showcontext *)data;

	nemotimer_set_timeout(timer, 500);

	nemotale_push_timer_event(scon->tale, time_current_msecs());
}

static void nemoshow_dispatch_tale_event(struct nemotale *tale, struct talenode *node, struct taleevent *event)
{
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_event != NULL) {
		show->dispatch_event(show, event);
	} else {
		if (nemotale_dispatch_grab(tale, event) == 0) {
			uint32_t id = nemotale_node_get_id(node);

			if (id != 0) {
				struct showone *one = (struct showone *)nemotale_node_get_data(node);
				struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

				if (canvas->dispatch_event != NULL)
					canvas->dispatch_event(show, one, event);
			}
		}
	}
}

struct nemoshow *nemoshow_create_view(struct nemotool *tool, int32_t width, int32_t height)
{
	struct showcontext *scon;
	struct nemoshow *show;
	struct nemotimer *timer;

	scon = (struct showcontext *)malloc(sizeof(struct showcontext));
	if (scon == NULL)
		return NULL;
	memset(scon, 0, sizeof(struct showcontext));

	scon->timer = timer = nemotimer_create(tool);
	if (timer == NULL)
		goto err1;
	nemotimer_set_callback(timer, nemoshow_dispatch_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, scon);

	scon->tool = tool;
	scon->width = width;
	scon->height = height;

	scon->egl = nemotool_create_egl(scon->tool);

	scon->eglcanvas = nemotool_create_egl_canvas(scon->egl, width, height);
	scon->canvas = NTEGL_CANVAS(scon->eglcanvas);

	nemocanvas_set_nemosurface(scon->canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(scon->canvas, nemoshow_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(scon->canvas, nemoshow_dispatch_canvas_frame);
	nemocanvas_set_dispatch_discard(scon->canvas, nemoshow_dispatch_canvas_discard);
	nemocanvas_set_dispatch_event(scon->canvas, nemoshow_dispatch_canvas_event);
	nemocanvas_set_dispatch_transform(scon->canvas, nemoshow_dispatch_canvas_transform);
	nemocanvas_set_dispatch_layer(scon->canvas, nemoshow_dispatch_canvas_layer);
	nemocanvas_set_dispatch_fullscreen(scon->canvas, nemoshow_dispatch_canvas_fullscreen);
	nemocanvas_set_dispatch_destroy(scon->canvas, nemoshow_dispatch_canvas_destroy);

	scon->tale = nemotale_create_gl();
	nemotale_set_backend(scon->tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(scon->egl),
				NTEGL_CONTEXT(scon->egl),
				NTEGL_CONFIG(scon->egl),
				(EGLNativeWindowType)NTEGL_WINDOW(scon->eglcanvas)));
	nemotale_set_dispatch_event(scon->tale, nemoshow_dispatch_tale_event);

	show = nemoshow_create();
	nemoshow_set_tale(show, scon->tale);
	nemoshow_set_size(show, width, height);
	nemoshow_set_context(show, scon);

	nemocanvas_set_min_size(scon->canvas,
			nemotale_get_minimum_width(scon->tale),
			nemotale_get_minimum_height(scon->tale));
	nemocanvas_set_max_size(scon->canvas,
			nemotale_get_maximum_width(scon->tale),
			nemotale_get_maximum_height(scon->tale));

	nemocanvas_set_userdata(NTEGL_CANVAS(scon->eglcanvas), scon->tale);
	nemotale_set_userdata(scon->tale, show);

	return show;

err1:
	free(scon);

	return NULL;
}

void nemoshow_destroy_view(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoshow_destroy(show);

	nemoshow_finalize();

	nemotale_destroy_gl(scon->tale);

	nemotool_destroy_egl_canvas(scon->eglcanvas);
	nemotool_destroy_egl(scon->egl);

	free(scon);
}

static void nemoshow_dispatch_destroy_view(void *data)
{
	struct nemoshow *show = (struct nemoshow *)data;

	nemoshow_destroy_view(show);
}

void nemoshow_destroy_view_on_idle(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemotool_dispatch_idle(scon->tool, nemoshow_dispatch_destroy_view, show);
}

void nemoshow_revoke_view(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocanvas_set_dispatch_event(scon->canvas, NULL);
}

void nemoshow_dispatch_frame(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocanvas_dispatch_frame(scon->canvas);
}

void nemoshow_dispatch_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocanvas_dispatch_resize(scon->canvas, width, height);
}

void nemoshow_dispatch_feedback(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocanvas_dispatch_feedback(scon->canvas);
}

void nemoshow_terminate_feedback(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocanvas_terminate_feedback(scon->canvas);
}

void nemoshow_view_set_layer(struct nemoshow *show, const char *layer)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	if (layer == NULL || strcmp(layer, "service") == 0)
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_SERVICE);
	else if (strcmp(layer, "background") == 0)
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	else if (strcmp(layer, "underlay") == 0)
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_UNDERLAY);
	else if (strcmp(layer, "overlay") == 0)
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_OVERLAY);
}

void nemoshow_view_put_layer(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;
}

void nemoshow_view_set_fullscreen_type(struct nemoshow *show, const char *type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	if (type == NULL)
		return;

	if (strcmp(type, "pick") == 0)
		nemocanvas_set_fullscreen_type(canvas, (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK));
	else if (strcmp(type, "pitch") == 0)
		nemocanvas_set_fullscreen_type(canvas, (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH));
}

void nemoshow_view_put_fullscreen_type(struct nemoshow *show, const char *type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	if (type == NULL)
		return;

	if (strcmp(type, "pick") == 0)
		nemocanvas_put_fullscreen_type(canvas, (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK));
	else if (strcmp(type, "pitch") == 0)
		nemocanvas_put_fullscreen_type(canvas, (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH));
}

void nemoshow_view_set_fullscreen(struct nemoshow *show, uint32_t id)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_fullscreen(canvas, id);
}

void nemoshow_view_put_fullscreen(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_put_fullscreen(canvas);
}

void nemoshow_view_set_parent(struct nemoshow *show, struct nemocanvas *parent)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_parent(canvas, parent);
}

void nemoshow_view_set_position(struct nemoshow *show, float x, float y)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_position(canvas, x, y);
}

void nemoshow_view_set_rotation(struct nemoshow *show, float r)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_rotation(canvas, r);
}

void nemoshow_view_set_scale(struct nemoshow *show, float sx, float sy)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_scale(canvas, sx, sy);
}

void nemoshow_view_set_pivot(struct nemoshow *show, float px, float py)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_pivot(canvas, px, py);
}

void nemoshow_view_put_pivot(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;
}

void nemoshow_view_set_anchor(struct nemoshow *show, float ax, float ay)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_anchor(canvas, ax, ay);
}

void nemoshow_view_set_flag(struct nemoshow *show, float fx, float fy)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_flag(canvas, fx, fy);
}

void nemoshow_view_set_opaque(struct nemoshow *show, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_opaque(canvas, x, y, width, height);
}

void nemoshow_view_set_min_size(struct nemoshow *show, float width, float height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_min_size(canvas, width, height);
}

void nemoshow_view_set_max_size(struct nemoshow *show, float width, float height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_max_size(canvas, width, height);
}

void nemoshow_view_set_state(struct nemoshow *show, const char *state)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_state(canvas, state);
}

void nemoshow_view_put_state(struct nemoshow *show, const char *state)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_put_state(canvas, state);
}

void nemoshow_view_set_region(struct nemoshow *show, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_region(canvas, x, y, width, height);
}

void nemoshow_view_put_region(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_put_region(canvas);
}

void nemoshow_view_set_scope(struct nemoshow *show, const char *fmt, ...)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;
	va_list vargs;
	char cmd[512];

	va_start(vargs, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, vargs);
	va_end(vargs);

	nemocanvas_set_scope(canvas, cmd);
}

void nemoshow_view_put_scope(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_put_scope(canvas);
}

void nemoshow_view_set_tag(struct nemoshow *show, uint32_t tag)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_set_tag(canvas, tag);
}

int nemoshow_view_move(struct nemoshow *show, uint32_t serial)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_move(canvas, serial);

	return 1;
}

int nemoshow_view_pick(struct nemoshow *show, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;
	uint32_t ptype = 0x0;

	if (type & NEMOSHOW_VIEW_PICK_ROTATE_TYPE)
		ptype |= (1 << NEMO_SURFACE_PICK_TYPE_ROTATE);
	if (type & NEMOSHOW_VIEW_PICK_SCALE_TYPE)
		ptype |= (1 << NEMO_SURFACE_PICK_TYPE_SCALE);
	if (type & NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE)
		ptype |= (1 << NEMO_SURFACE_PICK_TYPE_MOVE);

	nemocanvas_pick(canvas, serial0, serial1, ptype);

	return 1;
}

int nemoshow_view_pick_distant(struct nemoshow *show, void *event, uint32_t type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;
	int tap0, tap1;
	uint32_t ptype = 0x0;

	nemotale_event_get_distant_tapindices(show->tale, event, &tap0, &tap1);

	nemotale_event_set_used_on(event, tap0);
	nemotale_event_set_used_on(event, tap1);

	if (type & NEMOSHOW_VIEW_PICK_ROTATE_TYPE)
		ptype |= (1 << NEMO_SURFACE_PICK_TYPE_ROTATE);
	if (type & NEMOSHOW_VIEW_PICK_SCALE_TYPE)
		ptype |= (1 << NEMO_SURFACE_PICK_TYPE_SCALE);
	if (type & NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE)
		ptype |= (1 << NEMO_SURFACE_PICK_TYPE_MOVE);

	nemocanvas_pick(canvas,
			nemotale_event_get_serial_on(event, tap0),
			nemotale_event_get_serial_on(event, tap1),
			ptype);

	return 1;
}

void nemoshow_view_miss(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_miss(canvas);
}

void nemoshow_view_focus(struct nemoshow *show, uint32_t id)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemocanvas_focus(canvas, id);
}

void nemoshow_view_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemotool_resize_egl_canvas(scon->eglcanvas, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_update_one(show);
}

void nemoshow_view_redraw(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemocanvas *canvas = scon->canvas;

	nemoshow_render_one(show);

	nemotale_composite_egl_full(scon->tale);
}
