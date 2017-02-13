#include "libbb.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h>  /* for uname(2) */
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>

char* utoa_to_buf(unsigned n, char *buf, unsigned buflen)
{
	unsigned i, out, res;

	if (buflen) {
		out = 0;
		if (sizeof(n) == 4)
		// 2^32-1 = 4294967295
			i = 1000000000;
#if UINT_MAX > 4294967295 /* prevents warning about "const too large" */
		else
		if (sizeof(n) == 8)
		// 2^64-1 = 18446744073709551615
			i = 10000000000000000000;
#endif
		else
		for (; i; i /= 10) {
			res = n / i;
			n = n % i;
			if (res || out || i == 1) {
				if (--buflen == 0)
					break;
				out++;
				*buf++ = '0' + res;
			}
		}
	}
	return buf;
}

void write_pidfile(const char *path)
{
	int pid_fd, wrote_pidfile;
	char *end;
	char buf[sizeof(int)*3 + 2];
	struct stat sb;

	if (!path)
		return;
	/* we will overwrite stale pidfile */
	pid_fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (pid_fd < 0)
		return;

	/* path can be "/dev/null"! Test for such cases */
	wrote_pidfile = (fstat(pid_fd, &sb) == 0) && S_ISREG(sb.st_mode);

	if (wrote_pidfile) {
		/* few bytes larger, but doesn't use stdio */
		end = utoa_to_buf(getpid(), buf, sizeof(buf));
		*end = '\n';
		full_write(pid_fd, buf, end - buf + 1);
	}
	close(pid_fd);
}



/* Returns current kernel version encoded as major*65536 + minor*256 + patch,
 * so, for example,  to check if the kernel is greater than 2.2.11:
 *
 *     if (get_linux_version_code() > KERNEL_VERSION(2,2,11)) { <stuff> }
 */
int get_linux_version_code(void)
{
	struct utsname name;
	char *t;
	int i, r;

	uname(&name); /* never fails */
	t = name.release;
	r = 0;
	for (i = 0; i < 3; i++) {
		t = strtok(t, ".");
		r = r * 256 + (t ? atoi(t) : 0);
		t = NULL;
	}
	return r;
}

/* try to open up the specified device */
int device_open(const char *device, int mode)
{
	int m, f, fd;

	m = mode | O_NONBLOCK;

	/* Retry up to 5 times */
	/* TODO: explain why it can't be considered insane */
	for (f = 0; f < 5; f++) {
		fd = open(device, m, 0600);
		if (fd >= 0)
			break;
	}
	if (fd < 0)
		return fd;
	/* Reset original flags. */
	if (m != mode)
		fcntl(fd, F_SETFL, mode);
	return fd;
}


void bb_do_delay(int seconds)
{
	time_t start, now;

	start = time(NULL);
	do {
		sleep(seconds);
		now = time(NULL);
	} while ((now - start) < seconds);
}

/* Wrapper which restarts poll on EINTR or ENOMEM.
 * On other errors does perror("poll") and returns.
 * Warning! May take longer than timeout_ms to return! */
int safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout)
{
	while (1) {
		int n = poll(ufds, nfds, timeout);
		if (n >= 0)
			return n;
		/* Make sure we inch towards completion */
		if (timeout > 0)
			timeout--;
		/* E.g. strace causes poll to return this */
		if (errno == EINTR)
			continue;
		/* Kernel is very low on memory. Retry. */
		/* I doubt many callers would handle this correctly! */
		if (errno == ENOMEM)
			continue;
		perror("poll");
		return n;
	}
}

ssize_t safe_write(int fd, const void *buf, size_t count)
{
	ssize_t n;

	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

/*
 * Write all of the supplied buffer out to a file.
 * This does multiple writes as necessary.
 * Returns the amount written, or -1 on an error.
 */
ssize_t full_write(int fd, const void *buf, size_t len)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (len) {
		cc = safe_write(fd, buf, len);

		if (cc < 0) {
			if (total) {
				/* we already wrote some! */
				/* user can do another write to know the error code */
				return total;
			}
			return cc;  /* write() returns -1 on failure. */
		}

		total += cc;
		buf = ((const char *)buf) + cc;
		len -= cc;
	}

	return total;
}

ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;

	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

/*
 * Read all of the supplied buffer from a file.
 * This does multiple reads as necessary.
 * Returns the amount read, or -1 on an error.
 * A short read is returned on an end of file.
 */
ssize_t full_read(int fd, void *buf, size_t len)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (len) {
		cc = safe_read(fd, buf, len);

		if (cc < 0) {
			if (total) {
				/* we already have some! */
				/* user can do another read to know the error code */
				return total;
			}
			return cc; /* read() returns -1 on failure. */
		}
		if (cc == 0)
			break;
		buf = ((char *)buf) + cc;
		total += cc;
		len -= cc;
	}

	return total;
}

ssize_t read_close(int fd, void *buf, size_t size)
{
	/*int e;*/
	size = full_read(fd, buf, size);
	/*e = errno;*/
	close(fd);
	/*errno = e;*/
	return size;
}

ssize_t open_read_close(const char *filename, void *buf, size_t size)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
		return fd;
	return read_close(fd, buf, size);
}

