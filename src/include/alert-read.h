typedef struct alert_container alert_container_t;

int prelude_alert_read_msg(alert_container_t *ac, uint8_t *tag, uint32_t *len, unsigned char **buf);

alert_container_t *prelude_alert_read(int fd, uint8_t *tag);










