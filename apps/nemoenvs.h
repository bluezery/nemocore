#ifndef	__NEMOTOOL_ENVS_H__
#define	__NEMOTOOL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemobox.h>
#include <nemoitem.h>
#include <nemolist.h>

struct nemoaction {
	char *path;
	char *icon;
	char *ring;
	char *user;
	char *text;

	char *type;
	char *keypad;
	char *layer;
	char *network;
	char *resize;
};

struct nemogroup {
	struct nemoaction **actions;
	int sactions, nactions;

	char *path;
	char *icon;
	char *ring;

	char *type;
};

struct nemoenvs {
	struct nemoitem *configs;

	struct nemogroup **groups;
	int sgroups, ngroups;
};

extern struct nemoenvs *nemoenvs_create(void);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);
extern void nemoenvs_load_actions(struct nemoenvs *envs);

static inline int nemoenvs_get_groups_count(struct nemoenvs *envs)
{
	return envs->ngroups;
}

static inline int nemoenvs_get_actions_count(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->nactions;
}

static inline struct nemogroup *nemoenvs_get_group(struct nemoenvs *envs, int group)
{
	return envs->groups[group];
}

static inline struct nemoaction *nemoenvs_get_action(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action];
}

static inline const char *nemoenvs_get_group_path(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->path;
}

static inline const char *nemoenvs_get_group_icon(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->icon;
}

static inline const char *nemoenvs_get_group_ring(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->ring;
}

static inline int nemoenvs_is_group_type(struct nemoenvs *envs, int group, const char *type)
{
	return envs->groups[group]->type != NULL && strcmp(envs->groups[group]->type, type) == 0;
}

static inline const char *nemoenvs_get_action_icon(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->icon;
}

static inline const char *nemoenvs_get_action_ring(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->ring;
}

static inline const char *nemoenvs_get_action_text(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->text;
}

static inline int nemoenvs_is_action_type(struct nemoenvs *envs, int group, int action, const char *type)
{
	return envs->groups[group]->actions[action]->type != NULL && strcmp(envs->groups[group]->actions[action]->type, type) == 0;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
