#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <compz.h>
#include <shell.h>
#include <nemoshell.h>
#include <canvas.h>
#include <subcanvas.h>
#include <view.h>
#include <move.h>
#include <pick.h>
#include <picker.h>
#include <screen.h>
#include <seat.h>
#include <viewanimation.h>
#include <nemomisc.h>

static void nemo_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);
	uint32_t serial = ++bin->next_serial;

	nemo_surface_send_configure(bin->resource, width, height, serial);
}

static void nemo_send_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_transform(bin->resource, visible, x, y, width, height);
}

static void nemo_send_layer(struct nemocanvas *canvas, int visible)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_layer(bin->resource, visible);
}

static void nemo_send_fullscreen(struct nemocanvas *canvas, int id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_fullscreen(bin->resource, id, x, y, width, height);
}

static struct nemoclient nemo_client = {
	nemo_send_configure,
	nemo_send_transform,
	nemo_send_layer,
	nemo_send_fullscreen
};

static void nemo_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void nemo_surface_set_tag(struct wl_client *client, struct wl_resource *resource, uint32_t tag)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_tag(bin->view, tag);
}

static void nemo_surface_set_state(struct wl_client *client, struct wl_resource *resource, const char *state)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (strcmp(state, "catch") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_CATCH_STATE);
	else if (strcmp(state, "pick") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_PICK_STATE);
	else if (strcmp(state, "keypad") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_KEYPAD_STATE);
	else if (strcmp(state, "sound") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_SOUND_STATE);
	else if (strcmp(state, "layer") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_LAYER_STATE);
	else if (strcmp(state, "push") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_PUSH_STATE);
	else if (strcmp(state, "opaque") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_OPAQUE_STATE);
}

static void nemo_surface_put_state(struct wl_client *client, struct wl_resource *resource, const char *state)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (strcmp(state, "catch") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_CATCH_STATE);
	else if (strcmp(state, "pick") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_PICK_STATE);
	else if (strcmp(state, "keypad") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_KEYPAD_STATE);
	else if (strcmp(state, "sound") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_SOUND_STATE);
	else if (strcmp(state, "layer") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_LAYER_STATE);
	else if (strcmp(state, "push") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_PUSH_STATE);
	else if (strcmp(state, "opaque") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_OPAQUE_STATE);
}

static void nemo_surface_set_size(struct wl_client *client, struct wl_resource *resource, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->view->geometry.width = width;
	bin->view->geometry.height = height;
}

static void nemo_surface_set_min_size(struct wl_client *client, struct wl_resource *resource, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->min_width = width;
	bin->min_height = height;
}

static void nemo_surface_set_max_size(struct wl_client *client, struct wl_resource *resource, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->max_width = width;
	bin->max_height = height;
}

static void nemo_surface_set_position(struct wl_client *client, struct wl_resource *resource, wl_fixed_t x, wl_fixed_t y)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_position(bin->view,
			wl_fixed_to_double(x),
			wl_fixed_to_double(y));
}

static void nemo_surface_set_rotation(struct wl_client *client, struct wl_resource *resource, wl_fixed_t r)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_rotation(bin->view,
			wl_fixed_to_double(r));
}

static void nemo_surface_set_scale(struct wl_client *client, struct wl_resource *resource, wl_fixed_t sx, wl_fixed_t sy)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_scale(bin->view,
			wl_fixed_to_double(sx),
			wl_fixed_to_double(sy));
}

static void nemo_surface_set_pivot(struct wl_client *client, struct wl_resource *resource, int32_t px, int32_t py)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_pivot(bin->view, px, py);
}

static void nemo_surface_set_anchor(struct wl_client *client, struct wl_resource *resource, wl_fixed_t ax, wl_fixed_t ay)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_anchor(bin->view,
			wl_fixed_to_double(ax),
			wl_fixed_to_double(ay));
}

static void nemo_surface_set_flag(struct wl_client *client, struct wl_resource *resource, wl_fixed_t fx, wl_fixed_t fy)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_flag(bin->view,
			wl_fixed_to_double(fx),
			wl_fixed_to_double(fy));
}

static void nemo_surface_set_layer(struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (type == NEMO_SURFACE_LAYER_TYPE_BACKGROUND) {
		bin->layer = &bin->shell->background_layer;

		nemoview_put_state(bin->view, NEMOVIEW_CATCH_STATE);
	} else if (type == NEMO_SURFACE_LAYER_TYPE_SERVICE) {
		bin->layer = &bin->shell->service_layer;
	} else if (type == NEMO_SURFACE_LAYER_TYPE_OVERLAY) {
		bin->layer = &bin->shell->overlay_layer;
	} else if (type == NEMO_SURFACE_LAYER_TYPE_UNDERLAY) {
		bin->layer = &bin->shell->underlay_layer;
	}
}

static void nemo_surface_set_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent_resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct shellbin *pbin = (struct shellbin *)wl_resource_get_user_data(parent_resource);

	nemoshell_set_parent_bin(bin, pbin);
}

static void nemo_surface_set_region(struct wl_client *client, struct wl_resource *resource, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_region(bin->view, x, y, width, height);
}

static void nemo_surface_put_region(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_put_region(bin->view);
}

static void nemo_surface_set_scope(struct wl_client *client, struct wl_resource *resource, const char *cmd)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_scope(bin->view, cmd);
}

static void nemo_surface_put_scope(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_put_scope(bin->view);
}

static void nemo_surface_set_fullscreen_type(struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (type & (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK)) {
		bin->on_pickscreen = 1;
	}

	if (type & (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH)) {
		bin->on_pitchscreen = 1;
	}
#endif
}

static void nemo_surface_put_fullscreen_type(struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (type & (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK)) {
		bin->on_pickscreen = 0;
	}

	if (type & (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH)) {
		bin->on_pitchscreen = 0;
	}
#endif
}

static void nemo_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (bin->flags & NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(shell, id);
		if (screen != NULL) {
			if (bin->grabbed > 0)
				wl_signal_emit(&bin->ungrab_signal, bin);

			nemoshell_set_fullscreen_bin(shell, bin, screen);

			if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
				nemoseat_set_keyboard_focus(shell->compz->seat, bin->view);
				nemoseat_set_pointer_focus(shell->compz->seat, bin->view);
				nemoseat_set_stick_focus(shell->compz->seat, bin->view);
			}
		}
	}
#endif
}

static void nemo_surface_put_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (bin->flags & NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG)
		nemoshell_put_fullscreen_bin(shell, bin);
}

static void nemo_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMOSHELL_SURFACE_MOVABLE_FLAG) {
		nemoshell_move_canvas(bin->shell, bin, serial);
	}
}

static void nemo_surface_pick(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMOSHELL_SURFACE_PICKABLE_FLAG) {
		nemoshell_pick_canvas(bin->shell, bin, serial0, serial1, type);
	}
}

static void nemo_surface_miss(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);
}

static void nemo_surface_focus(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	nemoview_set_focus(bin->view, nemocompz_get_view_by_id(shell->compz, id));
}

static void nemo_surface_focus_on(struct wl_client *client, struct wl_resource *resource, wl_fixed_t x, wl_fixed_t y)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;
	float tx, ty;
	float sx, sy;

	nemoview_transform_to_global(bin->view,
			wl_fixed_to_double(x),
			wl_fixed_to_double(y),
			&tx, &ty);

	nemoview_set_focus(bin->view,
			nemocompz_pick_view(shell->compz, tx, ty, &sx, &sy, NEMOVIEW_PICK_STATE | NEMOVIEW_CANVAS_STATE | NEMOVIEW_KEYPAD_STATE));
}

static void nemo_surface_execute_command(struct wl_client *client, struct wl_resource *resource, const char *name, const char *cmds, uint32_t type, uint32_t coords, wl_fixed_t x, wl_fixed_t y, wl_fixed_t r)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (shell->execute_command != NULL) {
		float tx, ty;
		float tr;

		if (coords == NEMO_SURFACE_COORDINATE_TYPE_LOCAL) {
			nemoview_transform_to_global(bin->view,
					wl_fixed_to_double(x),
					wl_fixed_to_double(y),
					&tx, &ty);

			tr = wl_fixed_to_double(r) * M_PI / 180.0f + bin->view->geometry.r;
		} else {
			tx = wl_fixed_to_double(x);
			ty = wl_fixed_to_double(y);
			tr = wl_fixed_to_double(r) * M_PI / 180.0f;
		}

		shell->execute_command(shell->userdata, bin,
				name, cmds, type,
				tx, ty, tr);
	}
}

static void nemo_surface_execute_action(struct wl_client *client, struct wl_resource *resource, uint32_t group, uint32_t action, uint32_t type, uint32_t coords, wl_fixed_t x, wl_fixed_t y, wl_fixed_t r)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (shell->execute_action != NULL) {
		float tx, ty;
		float tr;

		if (coords == NEMO_SURFACE_COORDINATE_TYPE_LOCAL) {
			nemoview_transform_to_global(bin->view,
					wl_fixed_to_double(x),
					wl_fixed_to_double(y),
					&tx, &ty);

			tr = wl_fixed_to_double(r) * M_PI / 180.0f + bin->view->geometry.r;
		} else {
			tx = wl_fixed_to_double(x);
			ty = wl_fixed_to_double(y);
			tr = wl_fixed_to_double(r) * M_PI / 180.0f;
		}

		shell->execute_action(shell->userdata, bin,
				group, action, type,
				tx, ty, tr);
	}
}

static void nemo_surface_execute_content(struct wl_client *client, struct wl_resource *resource, uint32_t type, const char *path, uint32_t coords, wl_fixed_t x, wl_fixed_t y, wl_fixed_t r)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (shell->execute_content != NULL) {
		float tx, ty;
		float tr;

		if (coords == NEMO_SURFACE_COORDINATE_TYPE_LOCAL) {
			nemoview_transform_to_global(bin->view,
					wl_fixed_to_double(x),
					wl_fixed_to_double(y),
					&tx, &ty);

			tr = wl_fixed_to_double(r) * M_PI / 180.0f + bin->view->geometry.r;
		} else {
			tx = wl_fixed_to_double(x);
			ty = wl_fixed_to_double(y);
			tr = wl_fixed_to_double(r) * M_PI / 180.0f;
		}

		shell->execute_content(shell->userdata, bin,
				type, path,
				tx, ty, tr);
	}
}

static void nemo_surface_commit_serial(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->done_serial = serial;
}

static const struct nemo_surface_interface nemo_surface_implementation = {
	nemo_surface_destroy,
	nemo_surface_set_tag,
	nemo_surface_set_state,
	nemo_surface_put_state,
	nemo_surface_set_size,
	nemo_surface_set_min_size,
	nemo_surface_set_max_size,
	nemo_surface_set_position,
	nemo_surface_set_rotation,
	nemo_surface_set_scale,
	nemo_surface_set_pivot,
	nemo_surface_set_anchor,
	nemo_surface_set_flag,
	nemo_surface_set_layer,
	nemo_surface_set_parent,
	nemo_surface_set_region,
	nemo_surface_put_region,
	nemo_surface_set_scope,
	nemo_surface_put_scope,
	nemo_surface_set_fullscreen_type,
	nemo_surface_put_fullscreen_type,
	nemo_surface_set_fullscreen,
	nemo_surface_put_fullscreen,
	nemo_surface_move,
	nemo_surface_pick,
	nemo_surface_miss,
	nemo_surface_focus,
	nemo_surface_focus_on,
	nemo_surface_execute_command,
	nemo_surface_execute_action,
	nemo_surface_execute_content,
	nemo_surface_commit_serial
};

static void nemoshell_unbind_nemo_surface(struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->resource = NULL;
}

static void nemo_use_unstable_version(struct wl_client *client, struct wl_resource *resource, int32_t version)
{
	if (version > 1) {
		wl_resource_post_error(resource, 1, "nemo-shell: version not implemented yet");
		return;
	}
}

static void nemo_get_nemo_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, uint32_t type)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = sc->shell;
	struct shellbin *bin;

	if (nemoshell_get_bin(canvas)) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wayland shell surface is already registered");
		return;
	}

	bin = nemoshell_create_bin(shell, canvas, &nemo_client);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	nemoview_put_state(bin->view, NEMOVIEW_CATCH_STATE);

	if (shell->default_layer != NULL)
		bin->layer = shell->default_layer;

	bin->owner = sc;

	wl_client_get_credentials(client, &bin->pid, NULL, NULL);

	bin->resource = wl_resource_create(client, &nemo_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &nemo_surface_implementation, bin, nemoshell_unbind_nemo_surface);

	if (type == NEMO_SHELL_SURFACE_TYPE_NORMAL) {
		bin->type = NEMOSHELL_SURFACE_NORMAL_TYPE;

		nemoshell_use_client_state(shell, bin);
	} else if (type == NEMO_SHELL_SURFACE_TYPE_OVERLAY) {
		bin->type = NEMOSHELL_SURFACE_OVERLAY_TYPE;
		bin->view->transform.type = NEMOVIEW_TRANSFORM_OVERLAY;
	}
}

static void nemo_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	nemoshell_pong(sc, serial);
}

static const struct nemo_shell_interface nemo_implementation = {
	nemo_use_unstable_version,
	nemo_get_nemo_surface,
	nemo_pong
};

static int nemoshell_dispatch(const void *implementation, void *target, uint32_t opcode, const struct wl_message *message, union wl_argument *args)
{
	struct wl_resource *resource = (struct wl_resource *)target;
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	if (opcode != 0) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"must call use_unstable_version first");
		return 0;
	}

#define	NEMOSERVER_VERSION		1

	if (args[0].i != NEMOSERVER_VERSION) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"incompatible version, server is %d client wants %d", NEMOSERVER_VERSION, args[0].i);
		return 0;
	}

	wl_resource_set_implementation(resource, &nemo_implementation, sc, NULL);
}

int nemoshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct shellclient *sc;

	sc = nemoshell_create_client(client, shell, &nemo_shell_interface, id);
	if (sc == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_dispatcher(sc->resource, nemoshell_dispatch, NULL, sc, NULL);

	return 0;
}

int nemoshell_is_nemo_surface(struct shellbin *bin)
{
	return bin && bin->resource && wl_resource_instance_of(bin->resource, &nemo_surface_interface, &nemo_surface_implementation);
}

int nemoshell_is_nemo_surface_for_canvas(struct nemocanvas *canvas)
{
	struct nemocanvas *parent;
	struct shellbin *pbin;

	if (canvas == NULL)
		return 0;

	parent = nemosubcanvas_get_main_canvas(canvas);
	if (parent == NULL)
		return 0;

	pbin = nemoshell_get_bin(parent);
	if (pbin != NULL && pbin->resource != NULL &&
			wl_resource_instance_of(pbin->resource, &nemo_surface_interface, &nemo_surface_implementation))
		return 1;

	return 0;
}
