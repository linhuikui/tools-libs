#ifndef _LIBBB_H_
#define _LIBBB_H_
/*ios*/
char* utoa_to_buf(unsigned n, char *buf, unsigned buflen);
void write_pidfile(const char *path);

/* Returns current kernel version encoded as major*65536 + minor*256 + patch,
 * so, for example,  to check if the kernel is greater than 2.2.11:
 *
 *     if (get_linux_version_code() > KERNEL_VERSION(2,2,11)) { <stuff> }
 */
int get_linux_version_code(void);
/* try to open up the specified device */
int device_open(const char *device, int mode);
void bb_do_delay(int seconds);

/*poll*/
/* Wrapper which restarts poll on EINTR or ENOMEM.
 * On other errors does perror("poll") and returns.
 * Warning! May take longer than timeout_ms to return! */
int safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout);

/*ios*/
ssize_t safe_write(int fd, const void *buf, size_t count);
/*
 * Write all of the supplied buffer out to a file.
 * This does multiple writes as necessary.
 * Returns the amount written, or -1 on an error.
 */
ssize_t full_write(int fd, const void *buf, size_t len);
ssize_t safe_read(int fd, void *buf, size_t count);
/*
 * Read all of the supplied buffer from a file.
 * This does multiple reads as necessary.
 * Returns the amount read, or -1 on an error.
 * A short read is returned on an end of file.
 */
ssize_t full_read(int fd, void *buf, size_t len);
ssize_t read_close(int fd, void *buf, size_t size);
ssize_t open_read_close(const char *filename, void *buf, size_t size);


#endif //_LIBBB_H_
