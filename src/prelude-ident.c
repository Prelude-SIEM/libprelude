#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "common.h"
#include "prelude-ident.h"


struct prelude_ident {
        int fd;
        uint64_t *ident;       
};
        


/**
 * prelude_ident_new:
 * @filename: Pointer to a filename where the ident should be stored.
 *
 * Create a new #prelude_ident_t object. The current ident is set to 0
 * if there was no ident associated with this file, or the current ident.
 *
 * Returns: a new #prelude_ident_t object, or NULL if an error occured.
 */
prelude_ident_t *prelude_ident_new(const char *filename) 
{
        int exist;
        prelude_ident_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        exist = access(filename, F_OK);
        
        new->fd = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if ( new->fd < 0 ) {
                log(LOG_ERR, "couldn't open %s.\n", filename);
                free(new);
                return NULL;
        }

        new->ident = mmap(0, sizeof(*new->ident), PROT_READ|PROT_WRITE, MAP_SHARED, new->fd, 0);
        if ( ! new->ident ) {
                log(LOG_ERR, "mmap failed.\n");
                close(new->fd);
                free(new);
                return NULL;
        }

        if ( exit < 0 )
                (*new->ident) = 0;

        return new;
}




/**
 * prelude_ident_inc:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Increment the ident associated with the #prelude_ident_t object.
 *
 * Returns: the new ident.
 */
uint64_t prelude_ident_inc(prelude_ident_t *ident) 
{
        return ++(*ident->ident);
}



/**
 * prelude_ident_dec:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Decrement the ident associated with the #prelude_ident_t object.
 *
 * Returns: the new ident.
 */
uint64_t prelude_ident_dec(prelude_ident_t *ident) 
{
        return --(*ident->ident);
}



/**
 * prelude_ident_destroy:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Destroy a #prelude_ident_t object.
 */
void prelude_ident_destroy(prelude_ident_t *ident) 
{
        int ret;
        
        ret = munmap(ident->ident, sizeof(*ident->ident));
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't unmap ident %p\n", ident->ident);
                return;
        }
        
        ret = close(ident->fd);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't close ident fd %d\n", ident->fd);
                return;
        }
        
        free(ident);
}



