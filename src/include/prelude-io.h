typedef struct prelude_io prelude_io_t;


/*
 * Object creation / destruction functions.
 */
prelude_io_t *prelude_io_new(int fd);

void prelude_io_destroy(prelude_io_t *pio);

#ifdef HAVE_SSL
prelude_io_t *prelude_io_ssl_new(SSL *ssl);
#endif



/*
 * IO operations.
 */
int prelude_io_close(prelude_io_t *pio);

ssize_t prelude_io_read(prelude_io_t *pio, void *buf, size_t count);

ssize_t prelude_io_write(prelude_io_t *pio, const void *buf, size_t count);


