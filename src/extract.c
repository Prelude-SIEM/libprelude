#include <stdio.h>
#include <inttypes.h>
#include <netinet/in.h>

#include "common.h"
#include "extract.h"


int extract_uint64(uint64_t *dst, void *buf, uint32_t blen) 
{
        if ( blen != sizeof(uint64_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint64: couldn't convert.\n");
                return -1;
        }

        ((uint32_t *) dst)[0] = ntohl(((uint32_t *) buf)[1]);
        ((uint32_t *) dst)[1] = ntohl(((uint32_t *) buf)[0]);
     
        return 0;
}




int extract_uint32(uint32_t *dst, void *buf, uint32_t blen) 
{
        if ( blen != sizeof(uint32_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint32: couldn't convert.\n");
                return -1;
        }

        *dst = ntohl(*(uint32_t *) buf);

        return 0;
}




int extract_uint16(uint16_t *dst, void *buf, uint32_t blen) 
{
        if ( blen != sizeof(uint16_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint16: couldn't convert.\n");
                return -1;
        }

        *dst = ntohs(*(uint16_t *) buf);

        return 0;
}




int extract_uint8(uint8_t *dst, void *buf, uint32_t blen) 
{
        if ( blen != sizeof(uint8_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint8: couldn't convert.\n");
                return -1;
        }

        *dst = *(uint8_t *) buf;

        return 0;
}




const char *extract_str(void *buf, uint32_t blen) 
{
        const char *str = buf;
        
        if ( str[blen - 1] != '\0' ) 
                return NULL;

        return buf;
}
