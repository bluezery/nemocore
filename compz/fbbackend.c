#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <fbbackend.h>
#include <fbnode.h>
#include <compz.h>
#include <nemomisc.h>
#include <nemolog.h>

struct nemobackend *fbbackend_create(struct nemocompz *compz, int index)
{
	struct fbbackend *fb;
	struct fbnode *node;
	const char *rendernode;

	fb = (struct fbbackend *)malloc(sizeof(struct fbbackend));
	if (fb == NULL)
		return NULL;
	memset(fb, 0, sizeof(struct fbbackend));

	fb->base.destroy = fbbackend_destroy;

	fb->compz = compz;
	fb->udev = udev_ref(compz->udev);

	rendernode = nemoitem_get_attr(compz->configs, index, "rendernode");
	if (rendernode == NULL)
		rendernode = "/dev/fb0";

	node = fb_create_node(compz, rendernode);
	if (node == NULL) {
		nemolog_error("FRAMEBUFFER", "failed to create framebuffer node %s\n", rendernode);
		goto err1;
	}

	wl_list_insert(&compz->backend_list, &fb->base.link);

	return &fb->base;

err1:
	free(fb);

	return NULL;
}

void fbbackend_destroy(struct nemobackend *base)
{
	struct fbbackend *fb = (struct fbbackend *)container_of(base, struct fbbackend, base);
	struct rendernode *node, *next;

	wl_list_remove(&base->link);

	wl_list_for_each_safe(node, next, &fb->compz->render_list, link) {
		fb_destroy_node((struct fbnode *)container_of(node, struct fbnode, base));
	}

	udev_unref(fb->udev);

	free(fb);
}
