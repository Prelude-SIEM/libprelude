typedef struct prelude_message_buffered prelude_msgbuf_t;

prelude_msgbuf_t *prelude_msgbuf_new(int async_send);

void prelude_msgbuf_close(prelude_msgbuf_t *msgbuf);

void prelude_msgbuf_mark_end(prelude_msgbuf_t *msgbuf);

void prelude_msgbuf_set(prelude_msgbuf_t *msgbuf, uint8_t tag, uint32_t len, const void *data);

void prelude_msgbuf_set_header(prelude_msgbuf_t *msgbuf, uint8_t tag, uint8_t priority);
