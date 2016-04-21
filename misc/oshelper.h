#ifndef __OS_HELPER_H__
#define	__OS_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int os_socketpair_cloexec(int domain, int type, int protocol, int *sv);
extern int os_epoll_create_cloexec(void);
extern int os_create_anonymous_file(off_t size);

extern int os_exist_path(const char *path);
extern pid_t os_execute_path(const char *path, char *const argv[]);
extern int os_load_path(const char *path, char **buffer, int *size);
extern int os_save_path(const char *path, char *buffer, int size);

extern int os_append_path(char *path, const char *name);
extern const char *os_get_file_extension(const char *name);
extern char *os_get_file_path(const char *name);

extern int os_set_nonblocking_mode(int fd);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
