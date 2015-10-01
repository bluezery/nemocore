#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <wayland-server.h>
#include <wayland-scaler-server-protocol.h>
#include <wayland-presentation-timing-server-protocol.h>

#include <compz.h>
#include <datadevice.h>
#include <canvas.h>
#include <actor.h>
#include <view.h>
#include <layer.h>
#include <region.h>
#include <scaler.h>
#include <presentation.h>
#include <subcompz.h>
#include <seat.h>
#include <session.h>
#include <task.h>
#include <clipboard.h>
#include <screen.h>
#include <animation.h>
#include <effect.h>
#include <fbbackend.h>
#include <waylandbackend.h>
#include <evdevbackend.h>
#include <tuiobackend.h>
#include <sound.h>
#include <nemomisc.h>
#include <nemoease.h>
#include <nemolog.h>

#ifdef NEMOUX_WITH_DRM
#include <drmbackend.h>
#endif

static void compositor_create_surface(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id)
{
	nemocanvas_create(client, compositor_resource, id);
}

static void compositor_create_region(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id)
{
	nemoregion_create(client, compositor_resource, id);
}

static const struct wl_compositor_interface compositor_implementation = {
	compositor_create_surface,
	compositor_create_region
};

static void nemocompz_bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_compositor_interface, MIN(version, 3), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &compositor_implementation, compz, NULL);
}

static void nemocompz_bind_subcompositor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemosubcompz_bind(client, data, version, id);
}

static void nemocompz_bind_scaler(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemoscaler_bind(client, data, version, id);
}

static void nemocompz_bind_presentation(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemopresentation_bind(client, data, version, id);
}

static int on_term_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;

	nemolog_error("COMPZ", "received %d term signal\n", signum);

	if (compz->display != NULL) {
		nemocompz_exit(compz);
	}

	return 1;
}

static int on_chld_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct nemotask *task;
	pid_t pid;
	int state;

	nemolog_message("COMPZ", "received %d child signal\n", signum);

	while (1) {
		pid = waitpid(-1, &state, WNOHANG);
		if (pid < 0)
			return 1;
		if (pid == 0)
			return 1;

		wl_list_for_each(task, &compz->task_list, link) {
			if (task->pid == pid)
				break;
		}

		if (&task->link != &compz->task_list) {
			wl_list_remove(&task->link);

			task->cleanup(task, state);
		}
	}

	return 1;
}

static int on_fault_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;

	nemolog_error("COMPZ", "received %d fault signal\n", signum);

	if (compz->display != NULL) {
		nemocompz_exit(compz);
	}

	debug_show_backtrace();

	return 1;
}

static int on_pipe_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;

	nemolog_error("COMPZ", "received %d fault signal\n", signum);

	return 1;
}

static int nemocompz_check_xdg_runtime_dir(void)
{
	char *dir = getenv("XDG_RUNTIME_DIR");
	struct stat st;

	if (dir == NULL) {
		setenv("XDG_RUNTIME_DIR", "/tmp", 1);

		dir = getenv("XDG_RUNTIME_DIR");
		if (dir == NULL)
			return -1;
	}

	if (stat(dir, &st) || !S_ISDIR(st.st_mode)) {
		return -1;
	}

	if ((st.st_mode & 0777) != 0700 || st.st_uid != getuid()) {
		return -1;
	}

	return 0;
}

static char *nemocompz_get_display_name(void)
{
	char *ename = getenv("WAYLAND_DISPLAY");
	char *dname;
	int i;

	for (i = 0; i < 8; i++) {
		asprintf(&dname, "wayland-%d", i);
		if (ename == NULL || strcmp(ename, dname) != 0) {
			break;
		}

		free(dname);
	}

	return dname;
}

static inline void nemocompz_dispatch_actor_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoactor *actor, *next;

	wl_list_for_each_safe(actor, next, &compz->frame_list, frame_link) {
		actor->dispatch_frame(actor, msecs);
	}
}

static inline void nemocompz_dispatch_animation_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoanimation *anim, *next;
	double progress;

	wl_list_for_each_safe(anim, next, &compz->animation_list, link) {
		if (msecs < anim->stime)
			continue;

		anim->frame_count++;

		if (msecs >= anim->etime) {
			progress = 1.0f;
		} else {
			progress = nemoease_get(&anim->ease, msecs - anim->stime, anim->duration);
		}

		anim->frame(anim, progress);

		if (msecs >= anim->etime) {
			wl_list_remove(&anim->link);
			wl_list_init(&anim->link);

			if (anim->done != NULL)
				anim->done(anim);
		}
	}
}

static inline void nemocompz_dispatch_effect_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoeffect *effect, *next;
	int done;

	wl_list_for_each_safe(effect, next, &compz->effect_list, link) {
		effect->frame_count++;

		done = effect->frame(effect, msecs - effect->ptime);
		if (done != 0) {
			wl_list_remove(&effect->link);
			wl_list_init(&effect->link);

			if (effect->done != NULL)
				effect->done(effect);
		} else {
			effect->ptime = msecs;
		}
	}
}

static int nemocompz_dispatch_frame_timeout(void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	uint32_t msecs = time_current_msecs();

	nemocompz_dispatch_actor_frame(compz, msecs);
	nemocompz_dispatch_animation_frame(compz, msecs);
	nemocompz_dispatch_effect_frame(compz, msecs);

	if (!wl_list_empty(&compz->frame_list) ||
			!wl_list_empty(&compz->animation_list) ||
			!wl_list_empty(&compz->effect_list)) {
		wl_event_source_timer_update(compz->frame_timer, compz->frame_timeout);
	} else {
		compz->frame_done = 1;
	}

	return 1;
}

void nemocompz_dispatch_frame(struct nemocompz *compz)
{
	if (compz->frame_done != 0) {
		wl_event_source_timer_update(compz->frame_timer, compz->frame_timeout);

		compz->frame_done = 0;
	}
}

static int nemocompz_dispatch_framerate_timeout(void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct nemoscreen *screen;
	uint32_t msecs = time_current_msecs();

	nemolog_message("COMPZ", "%fs frametime\n", (double)msecs / 1000.0f);

	wl_list_for_each(screen, &compz->screen_list, link) {
		nemolog_message("COMPZ", "  [%d:%d] screen %u frames...\n", screen->node->nodeid, screen->screenid, screen->frame_count);

		screen->frame_count = 0;
	}

	wl_event_source_timer_update(compz->framerate_timer, 1000);
}

struct nemocompz *nemocompz_create(void)
{
	struct nemocompz *compz;
	struct wl_display *display;
	struct wl_event_loop *loop;
	char *env;

	compz = (struct nemocompz *)malloc(sizeof(struct nemocompz));
	if (compz == NULL)
		return NULL;
	memset(compz, 0, sizeof(struct nemocompz));

	compz->configs = nemoitem_create(64);
	if (compz->configs == NULL)
		goto err1;

	display = wl_display_create();
	if (display == NULL)
		goto err1;

	nemocompz_check_xdg_runtime_dir();

	loop = wl_display_get_event_loop(display);

	compz->sigsrc[0] = wl_event_loop_add_signal(loop, SIGTERM, on_term_signal, compz);
	compz->sigsrc[1] = wl_event_loop_add_signal(loop, SIGINT, on_term_signal, compz);
	compz->sigsrc[2] = wl_event_loop_add_signal(loop, SIGQUIT, on_term_signal, compz);
	compz->sigsrc[3] = wl_event_loop_add_signal(loop, SIGCHLD, on_chld_signal, compz);
	compz->sigsrc[4] = wl_event_loop_add_signal(loop, SIGSEGV, on_fault_signal, compz);
	compz->sigsrc[5] = wl_event_loop_add_signal(loop, SIGPIPE, on_pipe_signal, compz);

	compz->display = display;
	compz->loop = loop;
	compz->state = NEMOCOMPZ_RUNNING_STATE;
	compz->use_pixman = 0;
	compz->screen_idpool = 0;
	compz->keyboard_ids = 0;
	compz->pointer_ids = 0;
	compz->touch_ids = NEMOCOMPZ_POINTER_MAX;
	compz->nodemax = NEMOCOMPZ_NODE_MAX;
	compz->name = nemocompz_get_display_name();

	wl_signal_init(&compz->destroy_signal);

	wl_list_init(&compz->key_binding_list);
	wl_list_init(&compz->button_binding_list);
	wl_list_init(&compz->touch_binding_list);

	wl_list_init(&compz->backend_list);
	wl_list_init(&compz->screen_list);
	wl_list_init(&compz->render_list);
	wl_list_init(&compz->evdev_list);
	wl_list_init(&compz->touch_list);
	wl_list_init(&compz->tuio_list);
	wl_list_init(&compz->virtuio_list);
	wl_list_init(&compz->animation_list);
	wl_list_init(&compz->effect_list);
	wl_list_init(&compz->task_list);
	wl_list_init(&compz->layer_list);
	wl_list_init(&compz->canvas_list);
	wl_list_init(&compz->actor_list);
	wl_list_init(&compz->feedback_list);

	wl_signal_init(&compz->session_signal);
	compz->session_active = 1;

	wl_signal_init(&compz->show_input_panel_signal);
	wl_signal_init(&compz->hide_input_panel_signal);
	wl_signal_init(&compz->update_input_panel_signal);

	wl_signal_init(&compz->create_surface_signal);
	wl_signal_init(&compz->activate_signal);
	wl_signal_init(&compz->transform_signal);
	wl_signal_init(&compz->kill_signal);

	nemolayer_prepare(&compz->cursor_layer, &compz->layer_list);

	pixman_region32_init(&compz->damage);
	pixman_region32_init(&compz->region);

	compz->udev = udev_new();
	if (compz->udev == NULL)
		goto err1;

	compz->session = nemosession_create(compz);
	if (compz->session == NULL)
		goto err1;

	if (!wl_global_create(compz->display, &wl_compositor_interface, 3, compz, nemocompz_bind_compositor))
		goto err1;

	if (!wl_global_create(compz->display, &wl_subcompositor_interface, 1, compz, nemocompz_bind_subcompositor))
		goto err1;

	if (!wl_global_create(compz->display, &wl_scaler_interface, 2, compz, nemocompz_bind_scaler))
		goto err1;

	if (!wl_global_create(compz->display, &presentation_interface, 1, compz, nemocompz_bind_presentation))
		goto err1;

	datadevice_manager_init(compz->display);
	wl_display_init_shm(compz->display);

	compz->seat = nemoseat_create(compz);
	if (compz->seat == NULL)
		goto err1;

	compz->sound = nemosound_create(compz);
	if (compz->sound == NULL)
		goto err1;

	clipboard_create(compz->seat);

	if (wl_display_add_socket(compz->display, compz->name))
		goto err1;

	compz->frame_timer = wl_event_loop_add_timer(compz->loop, nemocompz_dispatch_frame_timeout, compz);
	if (compz->frame_timer == NULL)
		goto err1;

	compz->frame_timeout = NEMOCOMPZ_DEFAULT_FRAME_TIMEOUT;
	compz->frame_done = 1;

	env = getenv("NEMOUX_FRAMERATE_LOG");
	if (env != NULL && strcmp(env, "ON") == 0) {
		compz->framerate_timer = wl_event_loop_add_timer(compz->loop, nemocompz_dispatch_framerate_timeout, compz);
		if (compz->framerate_timer == NULL)
			goto err1;

		wl_event_source_timer_update(compz->framerate_timer, 1000);
	}

	wl_list_init(&compz->frame_list);

	return compz;

err1:
	nemocompz_destroy(compz);

	return NULL;
}

void nemocompz_destroy(struct nemocompz *compz)
{
	struct nemobackend *backend, *bnext;
	struct nemoscreen *screen, *snext;
	int i;

	nemolog_message("COMPZ", "emit compz's destroy signal\n");

	wl_signal_emit(&compz->destroy_signal, compz);

	nemolog_message("COMPZ", "destroy all screens\n");

	wl_list_for_each_safe(screen, snext, &compz->screen_list, link) {
		if (screen->destroy != NULL)
			screen->destroy(screen);
	}

	nemolog_message("COMPZ", "destroy all backends\n");

	wl_list_for_each_safe(backend, bnext, &compz->backend_list, link) {
		if (backend->destroy != NULL)
			backend->destroy(backend);
	}

	pixman_region32_fini(&compz->damage);
	pixman_region32_fini(&compz->region);

	if (compz->udev != NULL)
		udev_unref(compz->udev);

	if (compz->seat != NULL)
		nemoseat_destroy(compz->seat);

	if (compz->sound != NULL)
		nemosound_destroy(compz->sound);

	if (compz->name != NULL)
		free(compz->name);

	nemolog_message("COMPZ", "destroy event sources\n");

	for (i = 0; i < ARRAY_LENGTH(compz->sigsrc); i++) {
		if (compz->sigsrc[i] != NULL)
			wl_event_source_remove(compz->sigsrc[i]);
	}

	if (compz->frame_timer != NULL)
		wl_event_source_remove(compz->frame_timer);

	if (compz->framerate_timer != NULL)
		wl_event_source_remove(compz->framerate_timer);

	nemolog_message("COMPZ", "destroy current session\n");

	if (compz->session != NULL)
		nemosession_destroy(compz->session);

	nemolog_message("COMPZ", "destroy wayland display\n");

	if (compz->display != NULL)
		wl_display_destroy(compz->display);

	if (compz->configs != NULL)
		nemoitem_destroy(compz->configs);

	free(compz);
}

int nemocompz_run(struct nemocompz *compz)
{
	if (compz->state == NEMOCOMPZ_RUNNING_STATE) {
		setenv("WAYLAND_DISPLAY", compz->name, 1);

		wl_display_run(compz->display);
	}

	return 0;
}

void nemocompz_exit(struct nemocompz *compz)
{
	compz->state = NEMOCOMPZ_EXIT_STATE;

	compz->session_active = 0;
	wl_signal_emit(&compz->session_signal, compz);

	wl_display_terminate(compz->display);
}

void nemocompz_destroy_clients(struct nemocompz *compz)
{
	struct nemocanvas *canvas;

	while (!wl_list_empty(&compz->canvas_list)) {
		canvas = (struct nemocanvas *)container_of(compz->canvas_list.next, struct nemocanvas, link);

		if (canvas != NULL && canvas->resource != NULL) {
			wl_client_destroy(wl_resource_get_client(canvas->resource));
		}
	}
}

void nemocompz_acculumate_damage(struct nemocompz *compz)
{
	struct nemoscreen *screen;
	struct nemolayer *layer;
	struct nemoview *view, *child;
	pixman_region32_t opaque;

	pixman_region32_init(&opaque);

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					nemoview_accumulate_damage(child, &opaque);
				}
			}

			nemoview_accumulate_damage(view, &opaque);
		}
	}

	pixman_region32_fini(&opaque);

	wl_list_for_each(screen, &compz->screen_list, link) {
		pixman_region32_union(&screen->damage, &screen->damage, &compz->damage);
		pixman_region32_intersect(&screen->damage, &screen->damage, &screen->region);
	}

	pixman_region32_clear(&compz->damage);
}

void nemocompz_flush_damage(struct nemocompz *compz)
{
	struct nemocanvas *canvas;
	struct nemoactor *actor;

	wl_list_for_each(canvas, &compz->canvas_list, link) {
		if (canvas->base.dirty != 0)
			nemocanvas_flush_damage(canvas);
	}

	wl_list_for_each(actor, &compz->actor_list, link) {
		if (actor->base.dirty != 0)
			nemoactor_flush_damage(actor);
	}
}

void nemocompz_make_current(struct nemocompz *compz)
{
	if (compz->renderer != NULL && compz->renderer->make_current != NULL)
		compz->renderer->make_current(compz->renderer);
}

struct nemoscreen *nemocompz_get_main_screen(struct nemocompz *compz)
{
	if (wl_list_empty(&compz->screen_list))
		return NULL;

	return (struct nemoscreen *)container_of(compz->screen_list.next, struct nemoscreen, link);
}

struct nemoscreen *nemocompz_get_screen_on(struct nemocompz *compz, float x, float y)
{
	struct nemoscreen *screen;

	wl_list_for_each(screen, &compz->screen_list, link) {
		if (pixman_region32_contains_point(&screen->region, x, y, NULL)) {
			return screen;
		}
	}

	return NULL;
}

struct nemoscreen *nemocompz_get_screen(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid)
{
	struct nemoscreen *screen;

	wl_list_for_each(screen, &compz->screen_list, link) {
		if (screen->screenid == screenid && screen->node->nodeid == nodeid) {
			return screen;
		}
	}

	return NULL;
}

int32_t nemocompz_get_scene_width(struct nemocompz *compz)
{
	pixman_box32_t *extents;

	extents = pixman_region32_extents(&compz->region);

	return extents->x2;
}

int32_t nemocompz_get_scene_height(struct nemocompz *compz)
{
	pixman_box32_t *extents;

	extents = pixman_region32_extents(&compz->region);

	return extents->y2;
}

void nemocompz_update_scene(struct nemocompz *compz)
{
	struct nemoscreen *screen;

	pixman_region32_clear(&compz->region);

	wl_list_for_each(screen, &compz->screen_list, link) {
		pixman_region32_union(&compz->region, &compz->region, &screen->region);
	}
}

void nemocompz_update_transform(struct nemocompz *compz)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (child->transform.dirty != 0) {
						nemoview_update_transform(child);
					}
				}
			}

			if (view->transform.dirty != 0) {
				nemoview_update_transform(view);
			}
		}
	}
}

void nemocompz_dispatch_animation(struct nemocompz *compz, struct nemoanimation *animation)
{
	if (animation->frame == NULL || animation->duration == 0)
		return;

	wl_list_insert(&compz->animation_list, &animation->link);

	animation->frame_count = 0;
	animation->stime = time_current_msecs() + animation->delay;
	animation->etime = animation->stime + animation->duration;

	nemocompz_dispatch_frame(compz);
}

void nemocompz_dispatch_effect(struct nemocompz *compz, struct nemoeffect *effect)
{
	if (effect->frame == NULL)
		return;

	wl_list_insert(&compz->effect_list, &effect->link);

	effect->frame_count = 0;
	effect->stime = time_current_msecs();
	effect->ptime = effect->stime;

	nemocompz_dispatch_frame(compz);
}

void nemocompz_set_frame_timeout(struct nemocompz *compz, uint32_t timeout)
{
	compz->frame_timeout = timeout;
}

struct nemoevent *nemocompz_get_main_event(struct nemocompz *compz)
{
	if (compz->event == NULL)
		compz->event = nemoevent_create(compz);

	return compz->event;
}

int nemocompz_set_presentation_clock(struct nemocompz *compz, clockid_t id)
{
	struct timespec ts;

	if (clock_gettime(id, &ts) < 0)
		return -1;

	compz->presentation_clock = id;

	return 0;
}

int nemocompz_set_presentation_clock_software(struct nemocompz *compz)
{
	static const clockid_t clocks[] = {
		CLOCK_MONOTONIC_RAW,
		CLOCK_MONOTONIC_COARSE,
		CLOCK_MONOTONIC,
		CLOCK_REALTIME_COARSE,
		CLOCK_REALTIME
	};
	int i;

	for (i = 0; i < ARRAY_LENGTH(clocks); i++) {
		if (nemocompz_set_presentation_clock(compz, clocks[i]) == 0)
			return 0;
	}

	return -1;
}

void nemocompz_get_presentation_clock(struct nemocompz *compz, struct timespec *ts)
{
	if (clock_gettime(compz->presentation_clock, ts) < 0) {
		ts->tv_sec = 0;
		ts->tv_nsec = 0;
	}
}

int nemocompz_is_running(struct nemocompz *compz)
{
	return compz->state == NEMOCOMPZ_RUNNING_STATE;
}
