#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <nemolog.h>
#include <nemomisc.h>

#ifdef NEMO_LOG_ON
static __thread uint64_t __nsecs;

static int nemologfile = -1;
static int nemologtype = 0;

void __attribute__((constructor(101))) nemolog_initialize(void)
{
	if (getenv("NEMOLOG_SOCKET_PATH") != NULL)
		nemolog_open_socket(getenv("NEMOLOG_SOCKET_PATH"));
	else if (getenv("NEMOLOG_FILE_PATH") != NULL)
		nemolog_open_file(getenv("NEMOLOG_FILE_PATH"));
	else
		nemolog_set_file(2);

	__nsecs = time_current_nsecs();
}

void __attribute__((destructor(101))) nemolog_finalize(void)
{
	nemolog_close_file();
}

int nemolog_open_file(const char *filepath)
{
	nemologfile = open(filepath, O_RDWR | O_CREAT, 0644);

	return nemologfile;
}

void nemolog_close_file(void)
{
	close(nemologfile);

	nemologfile = -1;
}

void nemolog_set_file(int fd)
{
	nemologfile = fd;
}

int nemolog_open_socket(const char *socketpath)
{
	struct sockaddr_un addr;
	socklen_t size, namesize;
	int soc;

	soc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (soc < 0)
		return -1;

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketpath);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (connect(soc, (struct sockaddr *)&addr, size) < 0)
		goto err1;

	nemologfile = soc;
	nemologtype = 1;

	return soc;

err1:
	close(soc);

	return -1;
}

static int nemolog_write(const char *syntax, const char *tag, double secs, const char *fmt, va_list vargs)
{
	char msg[1024];

	snprintf(msg, sizeof(msg), syntax, tag, secs);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, vargs);

	if (nemologtype == 1)
		return send(nemologfile, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);

	return write(nemologfile, msg, strlen(msg));
}

int nemolog_message(const char *tag, const char *fmt, ...)
{
	uint64_t nsecs = time_current_nsecs();
	va_list vargs;
	int r;

	if (nemologfile < 0)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;33m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - __nsecs) / 1000000000.0f, fmt, vargs);
	va_end(vargs);

	return r;
}

int nemolog_warning(const char *tag, const char *fmt, ...)
{
	uint64_t nsecs = time_current_nsecs();
	va_list vargs;
	int r;

	if (nemologfile < 0)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;30m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - __nsecs) / 1000000000.0f, fmt, vargs);
	va_end(vargs);

	return r;
}

int nemolog_error(const char *tag, const char *fmt, ...)
{
	uint64_t nsecs = time_current_nsecs();
	va_list vargs;
	int r;

	if (nemologfile < 0)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;31m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - __nsecs) / 1000000000.0f, fmt, vargs);
	va_end(vargs);

	return r;
}

void nemolog_checkpoint(void)
{
	__nsecs = time_current_nsecs();
}

#endif
