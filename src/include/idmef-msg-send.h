#define idmef_constant(x)                       \
        x;                                      \
        idmef_adjust_msg_size(sizeof((x)))    


#define idmef_string(x)                         \
        x;                                      \
        idmef_adjust_msg_size(strlen(x) + 1)    


void idmef_adjust_msg_size(size_t size);

int idmef_msg_send(idmef_message_t *idmef, uint8_t priority);
