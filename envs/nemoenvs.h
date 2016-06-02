#ifndef __NEMOSHELL_ENVS_H__
#define __NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoapps.h>
#include <nemomsg.h>

#include <nemobox.h>
#include <nemoitem.h>

struct nemoshell;

struct nemoenvs {
	struct nemoshell *shell;

	struct nemoitem *configs;

	struct nemolist app_list;

	struct nemomsg *msg;
	struct nemomonitor *monitor;
};

extern struct nemoenvs *nemoenvs_create(struct nemoshell *shell);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_listen(struct nemoenvs *envs, const char *ip, int port);
extern int nemoenvs_send(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, const char *content);

extern void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);

static inline int nemoenvs_set_callback(struct nemoenvs *envs, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_callback(envs->msg, callback, data);
}

static inline int nemoenvs_set_source_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_source_callback(envs->msg, name, callback, data);
}

static inline int nemoenvs_set_destination_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_destination_callback(envs->msg, name, callback, data);
}

static inline int nemoenvs_put_callback(struct nemoenvs *envs, nemomsg_callback_t callback)
{
	return nemomsg_put_callback(envs->msg, callback);
}

static inline int nemoenvs_put_source_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback)
{
	return nemomsg_put_source_callback(envs->msg, name, callback);
}

static inline int nemoenvs_put_destination_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback)
{
	return nemomsg_put_destination_callback(envs->msg, name, callback);
}

static inline void nemoenvs_set_data(struct nemoenvs *envs, void *data)
{
	nemomsg_set_data(envs->msg, data);
}

extern int nemoenvs_dispatch_nemoshell_message(struct nemoenvs *envs, const char *src, const char *cmd, const char *path, struct itemone *one, void *data);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
