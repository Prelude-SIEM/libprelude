#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#endif

#include "prelude-io.h"


struct prelude_io {
        union {
                int fd;
#ifdef HAVE_SSL
                SSL *ssl;
#endif
        } fd;

        int (*close)(prelude_io_t *pio);
        ssize_t (*read)(prelude_io_t *pio, void *buf, size_t count);
        ssize_t (*write)(prelude_io_t *pio, const void *buf, size_t count);
};



#ifdef HAVE_SSL

/*
 * Call the SSL read function.
 */
static ssize_t ssl_read(prelude_io_t *pio, void *buf, size_t count) 
{
        return SSL_read(pio->fd.ssl, buf, count);
}



/*
 * Call the SSL write function.
 */
static ssize_t ssl_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        return SSL_write(pio->fd.ssl, buf, count);
}



/*
 * Close the SSL session.
 */
static int ssl_close(prelude_io_t *pio) 
{
        int ret;

        ret = SSL_shutdown(pio->fd.ssl);
        if ( ret <= 0 )
                return -1;

        SSL_free(pio->fd.ssl);

        return 0;
}

#endif


/*
 * Call the standard read function. Also, handle EINTR.
 */
static ssize_t normal_read(prelude_io_t *pio, void *buf, size_t count) 
{
        int ret;

        do {
                ret = read(pio->fd.fd, buf, count);
        } while ( ret < 0 && errno == EINTR );

        return ret;
}



/*
 * Call the standard write function. Also, handle EINTR.
 */
static ssize_t normal_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        int ret;

        do {
                ret = write(pio->fd.fd, buf, count);
        } while ( ret < 0 && errno == EINTR );

        return ret;
}



/*
 * Call the standard close function. Also, handle EINTR.
 */
static int normal_close(prelude_io_t *pio) 
{
        int ret;

        do {
                ret = close(pio->fd.fd);
        } while ( ret < 0 && errno == EINTR );

        return ret;
}



/**
 * prelude_io_read:
 * @pio: Pointer to a #prelude_io_t object.
 * buf: Pointer to the buffer to store data into.
 * count: Number of bytes to read.
 *
 * prelude_io_read() attempts to read up to @count bytes from the
 * file descriptor identified by @pio into the buffer starting at @buf.
 *
 * If @count is zero, prelude_io_read() returns zero and has no other
 * results. If @count is greater than SSIZE_MAX, the result is unspecified.
 *
 * The case where the read function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: On success, the number of bytes read is returned (zero
 * indicates end of file). It is not an error if this number is
 * smaller than the number of bytes requested; this may happen
 * for example because fewer bytes are actually available right
 * now (maybe because we were close to end-of-file, or because we
 * are reading from a pipe, or from a terminal), or because read()
 * was interrupted by a signal.
 *
 * On error, -1 is returned, and errno is set appropriately. In this
 * case it is left unspecified whether the file position (if any) changes.
 */
ssize_t prelude_io_read(prelude_io_t *pio, void *buf, size_t count) 
{
        return pio->read(pio, buf, count);
}



/**
 * prelude_io_write:
 * @pio: Pointer to a #prelude_io_t object.
 * @buf: Pointer to the buffer to write data from.
 * @count: Number of bytes to write.
 *
 * prelude_io_write() writes up to @count bytes to the file descriptor
 * identified by @pio from the buffer starting at @buf. POSIX requires
 * that a read() which can be proved to occur after a write() has returned
 * returns the new data. Note that not all file systems are POSIX conforming.
 *
 * The case where the write() function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: On success, the number of bytes written are returned (zero
 * indicates nothing was written). On error, -1 is returned, and errno
 * is set appropriately. If @count is zero and the file descriptor refers
 * to a regular file, 0 will be returned without causing any other effect.
 * For a special file, the results are not portable.
 */
ssize_t prelude_io_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        return pio->write(pio, buf, count);
}



/**
 * prelude_io_close:
 * @pio: Pointer to a #prelude_io_t object.
 *
 * prelude_io_close() closes the file descriptor indentified by @pio,
 *
 * The case where the close() function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: zero on success, or -1 if an error occurred.
 */
int prelude_io_close(prelude_io_t *pio)
{
        return pio->close(pio);
}



#ifdef HAVE_SSL

/**
 * prelude_io_ssl_new:
 * @ssl: Pointer on the SSL structure holding the TLS/SSL connection data.
 *
 * Create a new prelude IO object for SSL connection.
 *
 * Returns: a #prelude_io_t object or NULL on error.
 */
prelude_io_t *prelude_io_ssl_new(SSL *ssl) 
{
        prelude_io_t *new;

        new = malloc(sizeof(*new));
        if ( ! new )
                return NULL;

        new->fd.ssl = ssl;

        new->read = ssl_read;
        new->write = ssl_write;
        new->close = ssl_close;
        
        return new;
}

#endif


/**
 * prelude_io_new:
 * @fd: File descriptor identifying the connection.
 *
 * Create a new prelude IO object for the connection associated to the FD.
 *
 * Returns: a #prelude_io_t object or NULL on error.
 */
prelude_io_t *prelude_io_new(int fd) 
{
        prelude_io_t *new;

        new = malloc(sizeof(*new));
        if ( ! new )
                return NULL;

        new->fd.fd = fd;

        new->read = normal_read;
        new->write = normal_write;
        new->close = normal_close;
        
        return new;
}




/**
 * prelude_io_destroy:
 * @pio: Pointer to a #prelude_io_t object.
 *
 * Destroy the @pio object.
 */
void prelude_io_destroy(prelude_io_t *pio) 
{
        free(pio);
}
