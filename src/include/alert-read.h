
typedef ssize_t (readfunc_t)(int fd, void *buf, size_t count);

typedef struct alert_container alert_container_t;

int prelude_alert_read_msg(alert_container_t *ac, uint8_t *tag, uint32_t *len, void **buf);

alert_container_t *prelude_alert_read(int fd, uint8_t *tag, readfunc_t *read);










