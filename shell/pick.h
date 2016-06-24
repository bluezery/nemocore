#ifndef	__NEMO_SHELL_PICK_H__
#define	__NEMO_SHELL_PICK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <grab.h>

struct nemoshell;
struct shellbin;
struct nemoactor;

struct shellgrab_pick {
	struct shellgrab base;

	uint32_t type;

	struct {
		float r;
	} rotate;

	struct {
		double distance;
	} scale;

	struct {
		double distance;
	} resize;

	struct {
		float dx, dy;
	} move;

	struct touchpoint *tp0, *tp1;

	uint32_t frameid;
	int done;

	struct shellgrab_pick *other;
};

struct actorgrab_pick {
	struct actorgrab base;

	uint32_t type;

	struct {
		float r;
	} rotate;

	struct {
		double distance;
	} scale;

	struct {
		float dx, dy;
	} move;

	struct touchpoint *tp0, *tp1;

	uint32_t frameid;
	int done;

	struct actorgrab_pick *other;
};

extern int nemoshell_pick_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct shellbin *bin);
extern int nemoshell_pick_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial0, uint32_t serial1, uint32_t type);

extern int nemoshell_pick_actor_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct nemoactor *actor);
extern int nemoshell_pick_actor(struct nemoshell *shell, struct nemoactor *actor, uint32_t serial0, uint32_t serial1, uint32_t type);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
