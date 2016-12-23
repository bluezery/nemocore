#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <linux/input.h>

#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <backend.h>
#include <screen.h>
#include <view.h>
#include <layer.h>
#include <content.h>
#include <session.h>
#include <binding.h>
#include <plugin.h>
#include <xserver.h>
#include <timer.h>
#include <picker.h>
#include <keymap.h>
#include <nemoxml.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemolog.h>
#include <nemoitem.h>
#include <nemomisc.h>

#include <nemoenvs.h>
#include <nemomirror.h>
#include <nemobus.h>
#include <nemodb.h>
#include <nemotoken.h>
#include <nemomisc.h>

struct minishell {
	struct nemocompz *compz;
	struct nemoshell *shell;
	struct nemoenvs *envs;

	struct nemobus *bus;
	struct wl_event_source *busfd;

	struct nemodb *db;

	struct {
		char *path;
		char *args;
	} keypad;

	struct {
		char *path;
		char *args;
	} content;

	struct {
		char *path;
		char *args;
	} chrome;
};

static void minishell_update_state(struct minishell *mini, struct itemone *one, struct clientstate *state)
{
	struct nemoview *view;
	const char *owner;

	clientstate_set_attrs(state, one);

	owner = nemoitem_one_get_attr(one, "owner");
	if (owner != NULL) {
		view = nemocompz_get_view_by_uuid(mini->shell->compz, owner);

		if (nemoitem_one_has_sattr(one, "coords", "global") == 0) {
			float x = nemoitem_one_get_fattr(one, "x", 0.0f);
			float y = nemoitem_one_get_fattr(one, "y", 0.0f);
			float r = nemoitem_one_get_fattr(one, "r", 0.0f) * M_PI / 180.0f;

			nemoview_transform_to_global(view, x, y, &x, &y);
			nemoview_rotate_to_global(view, r, &r);

			clientstate_set_fattr(state, "x", x);
			clientstate_set_fattr(state, "y", y);
			clientstate_set_fattr(state, "r", r * 180.0f / M_PI);
		}
	}
}

static int minishell_dispatch_keypad(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct nemocompz *compz = shell->compz;
	struct nemolayer *layer;
	struct nemoview *view;
	struct nemoview *focus;
	struct nemotoken *args;
	const char *keypadpath = mini->keypad.path;
	const char *keypadargs = mini->keypad.args;
	float sx, sy;
	float x, y, r;
	pid_t pid;

	x = nemoitem_one_get_fattr(one, "x", 0.0f);
	y = nemoitem_one_get_fattr(one, "y", 0.0f);
	r = nemoitem_one_get_fattr(one, "r", 0.0f) * M_PI / 180.0f;

	focus = nemocompz_pick_view(compz, x, y, &sx, &sy, NEMOVIEW_PICK_STATE | NEMOVIEW_CANVAS_STATE);
	if (focus != NULL && nemoview_has_state(focus, NEMOVIEW_KEYPAD_STATE) != 0) {
		args = nemotoken_create(keypadpath, strlen(keypadpath));
		if (keypadargs != NULL) {
			nemotoken_append_one(args, ';');
			nemotoken_append_format(args, keypadargs, focus->uuid, nemoitem_one_get_sattr(one, "language", "eng"));
		}
		nemotoken_divide(args, ';');
		nemotoken_update(args);

		pid = os_execute_path(keypadpath, nemotoken_get_tokens(args), NULL);
		if (pid > 0) {
			struct clientstate *state;

			nemoenvs_attach_client(mini->envs, pid, keypadpath);

			state = nemoshell_create_client_state(shell, pid);
			if (state != NULL) {
				clientstate_set_fattr(state, "x", x);
				clientstate_set_fattr(state, "x", y);
				clientstate_set_fattr(state, "r", focus->geometry.r);
			}
		}

		nemotoken_destroy(args);
	}

	return 0;
}

static int minishell_dispatch_xapp(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct clientstate *state;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *_args = nemoitem_one_get_attr(one, "args");

	state = nemoshell_create_client_state(shell, 0);
	if (state != NULL)
		minishell_update_state(mini, one, state);

	nemoenvs_launch_xapp(mini->envs, _path, _args, state);

	return 0;
}

static int minishell_dispatch_app(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct clientstate *state;
	struct nemotoken *args;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *_args = nemoitem_one_get_attr(one, "args");
	pid_t pid;

	args = nemotoken_create(_path, strlen(_path));
	if (_args != NULL)
		nemotoken_append_format(args, ";%s", _args);
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(_path, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		nemoenvs_attach_client(mini->envs, pid, _path);

		state = nemoshell_create_client_state(shell, pid);
		if (state != NULL)
			minishell_update_state(mini, one, state);
	}

	nemotoken_destroy(args);

	return 0;
}

static int minishell_dispatch_content(struct minishell *mini, struct itemone *one)
{
	struct nemotoken *args;
	struct clientstate *state;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *contentpath = mini->content.path;
	const char *contentargs = mini->content.args;
	pid_t pid;

	args = nemotoken_create(contentpath, strlen(contentpath));
	if (contentargs != NULL) {
		nemotoken_append_one(args, ';');
		nemotoken_append_format(args, contentargs, _path);
	}
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(contentpath, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		nemoenvs_attach_client(mini->envs, pid, contentpath);

		state = nemoshell_create_client_state(mini->shell, pid);
		if (state != NULL)
			minishell_update_state(mini, one, state);
	}

	nemotoken_destroy(args);

	return 0;
}

static int minishell_dispatch_bookmark(struct minishell *mini, struct itemone *one)
{
	struct nemotoken *args;
	struct clientstate *state;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *chromepath = mini->chrome.path;
	const char *chromeargs = mini->chrome.args;
	pid_t pid;

	args = nemotoken_create(chromepath, strlen(chromepath));
	if (chromeargs != NULL) {
		nemotoken_append_one(args, ';');
		nemotoken_append_format(args, chromeargs, _path);
	}
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(chromepath, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		nemoenvs_attach_client(mini->envs, pid, chromepath);

		state = nemoshell_create_client_state(mini->shell, pid);
		if (state != NULL)
			minishell_update_state(mini, one, state);
	}

	nemotoken_destroy(args);

	return 0;
}

static int minishell_dispatch_close(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct shellbin *bin;
	const char *uuid;

	uuid = nemoitem_one_get_attr(one, "uuid");
	if (uuid != NULL) {
		bin = nemoshell_get_bin_by_uuid(shell, uuid);
		if (bin != NULL)
			nemoshell_send_bin_close(bin);
	}

	return 0;
}

static int minishell_dispatch_close_all(struct minishell *mini, struct itemone *one)
{
	nemoenvs_terminate_clients(mini->envs);
	nemoenvs_terminate_xclients(mini->envs);

	return 0;
}

static int minishell_dispatch_command(struct minishell *mini, struct itemone *one)
{
	const char *type = nemoitem_one_get_attr(one, "type");

	if (strcmp(type, "keypad") == 0)
		minishell_dispatch_keypad(mini, one);
	else if (strcmp(type, "xapp") == 0)
		minishell_dispatch_xapp(mini, one);
	else if (strcmp(type, "app") == 0)
		minishell_dispatch_app(mini, one);
	else if (strcmp(type, "content") == 0)
		minishell_dispatch_content(mini, one);
	else if (strcmp(type, "bookmark") == 0)
		minishell_dispatch_bookmark(mini, one);
	else if (strcmp(type, "close") == 0)
		minishell_dispatch_close(mini, one);
	else if (strcmp(type, "close_all") == 0)
		minishell_dispatch_close_all(mini, one);

	return 0;
}

static int minishell_dispatch_config(struct minishell *mini, struct itemone *one)
{
	if (nemoitem_one_has_path_prefix(one, "/nemoshell/keypad") != 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");

		mini->keypad.path = path != NULL ? strdup(path) : NULL;
		mini->keypad.args = args != NULL ? strdup(args) : NULL;
	} else if (nemoitem_one_has_path_prefix(one, "/nemoshell/chrome") != 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");

		mini->chrome.path = path != NULL ? strdup(path) : NULL;
		mini->chrome.args = args != NULL ? strdup(args) : NULL;
	} else if (nemoitem_one_has_path_prefix(one, "/nemoshell/content") != 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");

		mini->content.path = path != NULL ? strdup(path) : NULL;
		mini->content.args = args != NULL ? strdup(args) : NULL;
	}

	return 0;
}

static int minishell_dispatch_db(struct minishell *mini, const char *configpath)
{
	struct itemone *one;
	struct dbiter *iter;

	mini->db = nemodb_create("mongodb://127.0.0.1");
	nemodb_use_collection(mini->db, "nemodb", configpath);

	iter = nemodb_query_iter_all(mini->db);

	while ((one = nemodb_iter_next(iter)) != NULL) {
		nemoenvs_set_config(mini->envs, one);

		minishell_dispatch_config(mini, one);

		nemoitem_one_destroy(one);
	}

	return 0;
}

static int minishell_dispatch_text(struct minishell *mini, const char *configpath)
{
	struct nemoitem *item;
	struct itemone *one;

	item = nemoitem_create();
	nemoitem_load_textfile(item, configpath, ' ');

	nemoitem_for_each(one, item) {
		nemoenvs_set_config(mini->envs, one);

		minishell_dispatch_config(mini, one);
	}

	nemoitem_destroy(item);

	return 0;
}

static int minishell_dispatch_bus(int fd, uint32_t mask, void *data)
{
	struct minishell *mini = (struct minishell *)data;
	struct nemoitem *msg;
	struct itemone *one;

	msg = nemobus_recv_item(mini->bus);
	if (msg == NULL)
		return 1;

	nemoitem_for_each(one, msg) {
		if (nemoitem_one_has_path(one, "/nemoshell/command") != 0)
			minishell_dispatch_command(mini, one);
	}

	nemoitem_destroy(msg);

	return 1;
}

static void minishell_destroy_client(void *data, pid_t pid)
{
	struct minishell *mini = (struct minishell *)data;

	if (nemoenvs_respawn_app(mini->envs, pid) > 0) {
	} else if (nemoenvs_detach_client(mini->envs, pid) != 0) {
	} else if (nemoenvs_detach_xclient(mini->envs, pid) != 0) {
	}
}

static void minishell_update_client(void *data, struct shellbin *bin, struct clientstate *state)
{
	struct minishell *mini = (struct minishell *)data;
	struct nemoshell *shell = mini->shell;

	if (nemoitem_one_has_attr(state->one, "mirrorscreen") != 0) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(shell, nemoitem_one_get_attr(state->one, "mirrorscreen"));
		if (screen != NULL && screen->dw != 0 && screen->dh != 0) {
			struct nemomirror *mirror;

			mirror = nemomirror_create(shell, screen->dx, screen->dy, screen->dw, screen->dh, "overlay");
			if (mirror != NULL) {
				nemoshell_kill_fullscreen_bin(shell, screen->target);

				nemomirror_set_view(mirror, bin->view);
				nemomirror_check_screen(mirror, screen);
			}
		}
	}
}

static void minishell_enter_idle(void *data)
{
	struct minishell *mini = (struct minishell *)data;

	nemoenvs_terminate_clients(mini->envs);
	nemoenvs_terminate_xclients(mini->envs);

	nemoenvs_execute_screensavers(mini->envs);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "seat",								required_argument,	NULL,		's' },
		{ "rendernode",					required_argument,	NULL,		'r' },
		{ "evdevopts",					required_argument,	NULL,		'e' },
		{ "xdisplay",						required_argument,	NULL,		'x' },
		{ "tty",								required_argument,	NULL,		't' },
		{ "config",							required_argument,	NULL,		'c' },
		{ "help",								no_argument,				NULL,		'h' },
		{ 0 }
	};

	struct minishell *mini;
	struct nemoshell *shell;
	struct nemocompz *compz;
	char *configpath = getenv("NEMOSHELL_CONFIG_PATH");
	char *rendernode = getenv("NEMOSHELL_RENDER_NODE");
	char *evdevopts = getenv("NEMOSHELL_EVDEV_OPTS");
	char *xdisplay = getenv("NEMOSHELL_XDISPLAY_NUMBER");
	char *seat = getenv("NEMOSHELL_SEAT_NAME");
	char *framelog = getenv("NEMOSHELL_FRAMELOG");
	int tty = 0;
	int opt;

	while (opt = getopt_long(argc, argv, "s:r:e:x:t:c:h", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				seat = strdup(optarg);
				break;

			case 'r':
				rendernode = strdup(optarg);
				break;

			case 'e':
				evdevopts = strdup(optarg);
				break;

			case 'x':
				xdisplay = strdup(optarg);
				break;

			case 't':
				tty = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			case 'h':
				fprintf(stderr, "usage: minishell --seat [name] --tty [number] --config [filepath]\n");
				return 0;

			default:
				break;
		}
	}

	if (configpath == NULL)
		asprintf(&configpath, "%s/.config/nemoshell.item", getenv("HOME"));

	mini = (struct minishell *)malloc(sizeof(struct minishell));
	if (mini == NULL)
		return -1;
	memset(mini, 0, sizeof(struct minishell));

	compz = nemocompz_create();
	if (compz == NULL)
		goto out1;
	nemocompz_load_backend(compz, "drm", rendernode);
	nemocompz_load_backend(compz, "evdev", evdevopts);

	shell = nemoshell_create(compz);
	if (shell == NULL)
		goto out2;
	nemoshell_set_destroy_client(shell, minishell_destroy_client);
	nemoshell_set_update_client(shell, minishell_update_client);
	nemoshell_set_enter_idle(shell, minishell_enter_idle);
	nemoshell_set_userdata(shell, mini);

	if (framelog != NULL && strcasecmp(framelog, "ON") == 0)
		nemoshell_set_frame_timeout(shell, 1000);

	mini->compz = compz;
	mini->shell = shell;

	mini->envs = nemoenvs_create(shell);

	if (configpath[0] == '@') {
		minishell_dispatch_db(mini, configpath + 1);
	} else {
		minishell_dispatch_text(mini, configpath);
	}

	mini->bus = nemobus_create();
	nemobus_connect(mini->bus, NULL);
	nemobus_advertise(mini->bus, "set", "/nemoshell");

	mini->busfd = wl_event_loop_add_fd(compz->loop,
			nemobus_get_socket(mini->bus),
			WL_EVENT_READABLE,
			minishell_dispatch_bus,
			mini);

	nemoenvs_launch_xserver(mini->envs, xdisplay == NULL ? 0 : strtoul(xdisplay, NULL, 10), rendernode);
	nemoenvs_use_xserver(mini->envs, xdisplay == NULL ? 0 : strtoul(xdisplay, NULL, 10));

	nemosession_connect(compz->session, seat, tty);

	nemocompz_add_key_binding(compz, KEY_F1, MODIFIER_CTRL, nemoenvs_handle_terminal_key, (void *)mini->envs);
	nemocompz_add_key_binding(compz, KEY_F2, MODIFIER_CTRL, nemoenvs_handle_touch_key, (void *)mini->envs);
	nemocompz_add_key_binding(compz, KEY_ESC, MODIFIER_CTRL, nemoenvs_handle_escape_key, (void *)mini->envs);
	nemocompz_add_button_binding(compz, BTN_LEFT, nemoenvs_handle_left_button, (void *)mini->envs);
	nemocompz_add_button_binding(compz, BTN_RIGHT, nemoenvs_handle_right_button, (void *)mini->envs);
	nemocompz_add_touch_binding(compz, nemoenvs_handle_touch_event, (void *)mini->envs);

	nemocompz_make_current(compz);

	nemoenvs_execute_backgrounds(mini->envs);
	nemoenvs_execute_daemons(mini->envs);

	nemocompz_run(compz);

	if (mini->db != NULL)
		nemodb_destroy(mini->db);

	nemoenvs_destroy(mini->envs);

	nemoshell_destroy(shell);

out2:
	nemocompz_destroy(compz);

out1:
	free(mini);

	return 0;
}