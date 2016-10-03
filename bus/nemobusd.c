#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <getopt.h>

#include <nemobus.h>
#include <nemohelper.h>
#include <nemolist.h>
#include <nemolog.h>
#include <nemomisc.h>

struct busclient {
	char *path;

	int soc;

	struct nemolist link;
};

struct nemobusd {
	struct nemolist client_list;
};

struct bustask;

typedef void (*nemobusd_dispatch_task)(int efd, struct bustask *task);

struct bustask {
	struct nemobusd *busd;

	int fd;

	nemobusd_dispatch_task dispatch;
};

static struct busclient *nemobusd_create_client(struct nemobusd *busd, int soc, const char *path)
{
	struct busclient *client;

	client = (struct busclient *)malloc(sizeof(struct busclient));
	if (client == NULL)
		return NULL;
	memset(client, 0, sizeof(struct busclient));

	client->path = strdup(path);
	client->soc = soc;

	nemolist_insert_tail(&busd->client_list, &client->link);

	nemolog_message("BUSD", "advertise(%d:%s)\n", soc, path);

	return client;
}

static void nemobusd_destroy_client(struct nemobusd *busd, int soc)
{
	struct busclient *client;

	nemolist_for_each(client, &busd->client_list, link) {
		if (client->soc == soc) {
			nemolist_remove(&client->link);

			free(client->path);
			free(client);
		}
	}

	nemolog_message("BUSD", "disconnect(%d)\n", soc);
}

static void nemobusd_dispatch_message_task(int efd, struct bustask *task)
{
	struct nemobusd *busd = task->busd;
	struct epoll_event ep;
	struct json_object *jobj;
	struct json_object *pobj;
	const char *path;
	char msg[4096];
	int len;

	len = read(task->fd, msg, sizeof(msg));
	if (len < 0) {
		if (errno == EAGAIN) {
			ep.events = EPOLLIN | EPOLLERR | EPOLLHUP;
			ep.data.ptr = (void *)task;
			epoll_ctl(efd, EPOLL_CTL_MOD, task->fd, &ep);
		} else {
			nemobusd_destroy_client(busd, task->fd);

			epoll_ctl(efd, EPOLL_CTL_DEL, task->fd, &ep);

			close(task->fd);

			free(task);
		}
	} else if (len == 0) {
		nemobusd_destroy_client(busd, task->fd);

		epoll_ctl(efd, EPOLL_CTL_DEL, task->fd, &ep);

		close(task->fd);

		free(task);
	} else {
		msg[len] = '\0';

		nemolog_message("BUSD", "recv(%d) [%s]\n", task->fd, msg);

		jobj = json_tokener_parse(msg);

		if (json_object_object_get_ex(jobj, "path", &pobj) == 0)
			goto out0;
		path = json_object_get_string(pobj);

		if (strcmp(path, "/nemobusd") == 0) {
			struct json_object *tobj;

			json_object_object_foreach(jobj, key, value) {
				if (strcmp(key, "path") == 0)
					continue;

				if (strcmp(key, "advertise") == 0) {
					if (json_object_object_get_ex(value, "path", &tobj) != 0) {
						nemobusd_create_client(busd, task->fd, json_object_get_string(tobj));

						json_object_put(tobj);
					}
				}
			}
		} else {
			struct busclient *client;

			json_object_object_foreach(jobj, key, value) {
				if (strcmp(key, "path") == 0)
					continue;

				nemolist_for_each(client, &busd->client_list, link) {
					if (namespace_has_prefix(client->path, path) != 0) {
						send(client->soc, msg, len, MSG_NOSIGNAL | MSG_DONTWAIT);

						nemolog_message("BUSD", "send(%s:%d) [%s]\n", client->path, client->soc, msg);
					}
				}
			}
		}

		json_object_put(pobj);
out0:
		json_object_put(jobj);
	}
}

static void nemobusd_dispatch_listen_task(int efd, struct bustask *task)
{
	struct nemobusd *busd = task->busd;
	struct busclient *client;
	struct sockaddr_un addr;
	struct bustask *ctask;
	struct epoll_event ep;
	socklen_t size;
	int csoc;

	csoc = accept(task->fd, (struct sockaddr *)&addr, &size);
	if (csoc <= 0)
		return;

	os_set_nonblocking_mode(csoc);

	ctask = (struct bustask *)malloc(sizeof(struct bustask));
	ctask->fd = csoc;
	ctask->busd = busd;
	ctask->dispatch = nemobusd_dispatch_message_task;

	os_epoll_add_fd(efd, csoc, EPOLLIN | EPOLLERR | EPOLLHUP, ctask);

	nemolog_message("BUSD", "connect(%d)\n", csoc);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "socketpath",				required_argument,		NULL,		's' },
		{ 0 }
	};
	struct sockaddr_un addr;
	socklen_t size, namesize;
	struct nemobusd *busd;
	struct bustask *task;
	struct epoll_event ep;
	struct epoll_event eps[16];
	char *socketpath = NULL;
	int efd;
	int neps;
	int lsoc;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "s:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				socketpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (socketpath == NULL)
		socketpath = strdup("/tmp/nemo.bus.0");
	unlink(socketpath);

	lsoc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (lsoc < 0)
		goto err0;

	os_set_nonblocking_mode(lsoc);

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketpath);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (bind(lsoc, (struct sockaddr *)&addr, size) < 0)
		goto err1;

	if (listen(lsoc, 16) < 0)
		goto err1;

	efd = os_epoll_create_cloexec();
	if (efd < 0)
		goto err1;

	busd = (struct nemobusd *)malloc(sizeof(struct nemobusd));
	nemolist_init(&busd->client_list);

	task = (struct bustask *)malloc(sizeof(struct bustask));
	task->fd = lsoc;
	task->busd = busd;
	task->dispatch = nemobusd_dispatch_listen_task;

	os_epoll_add_fd(efd, lsoc, EPOLLIN | EPOLLERR | EPOLLHUP, task);

	while (1) {
		neps = epoll_wait(efd, eps, ARRAY_LENGTH(eps), -1);

		for (i = 0; i < neps; i++) {
			task = eps[i].data.ptr;
			task->dispatch(efd, task);
		}
	}

	close(efd);

err1:
	close(lsoc);

err0:
	free(socketpath);

	return 0;
}
