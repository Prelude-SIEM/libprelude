int extract_uint64(uint64_t *dst, void *buf, uint32_t blen);

int extract_uint32(uint32_t *dst, void *buf, uint32_t blen);

int extract_uint16(uint16_t *dst, void *buf, uint32_t blen);

int extract_uint8(uint8_t *dst, void *buf, uint32_t blen);

const char *extract_str(void *buf, uint32_t blen);



#define extract_int(type, buf, blen, dst) do {        \
        int ret;                                      \
        ret = extract_ ## type (&dst, buf, blen);     \
        if ( ret < 0 )                                \
                return -1;                            \
} while (0)
           

#define extract_string(buf, blen, dst)                                    \
        dst = extract_str(buf, blen);                                     \
        if ( ! dst ) {                                                    \
               log(LOG_ERR, "Datatype error, buffer is not a string.\n"); \
               return -1;                                                 \
        }
