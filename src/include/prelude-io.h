typedef struct prelude_io prelude_io_t;

/*
 * Object creation / destruction functions.
 */
prelude_io_t *prelude_io_new(void);

void prelude_io_destroy(prelude_io_t *pio);


void prelude_io_set_file_io(prelude_io_t *pio, int fd);

void prelude_io_set_ssl_io(prelude_io_t *pio, void *ssl);

void prelude_io_set_socket_io(prelude_io_t *pio, int fd);




/*
 * IO operations.
 */
int prelude_io_close(prelude_io_t *pio);

ssize_t prelude_io_read(prelude_io_t *pio, void *buf, size_t count);

ssize_t prelude_io_read_delimited(prelude_io_t *pio, void **buf);

ssize_t prelude_io_write(prelude_io_t *pio, const void *buf, size_t count);

int prelude_io_write_delimited(prelude_io_t *pio, const void *buf, uint16_t count);

ssize_t prelude_io_forward(prelude_io_t *dst, prelude_io_t *src, size_t count);

