#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "common.h"
#include "prelude-log.h"
#include "prelude-client.h"
#include "tls-util.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*
 * can be:
 * -----BEGIN CERTIFICATE-----
 * -----BEGIN X509 CERTIFICATE-----
 */

#define END_WITHIN_LEN     (sizeof("X509 CERTIFICATE-----") - 1)
#define BEGIN_CERT_STR     "-----BEGIN "
#define END_CERT_STR       "CERTIFICATE-----"


 
static int cmp_certificate_issuer(gnutls_x509_crt crt, const char *dn, size_t dnsize)
{
        int ret;
        char buf[128];
        size_t size = sizeof(buf);
        
        ret = gnutls_x509_crt_get_issuer_dn(crt, buf, &size);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't get certificate issue dn: %s.\n", gnutls_strerror(ret));
                return -1;
        }
        
        if ( size != dnsize )
                return -1;
        
        ret = strcmp(buf, dn);

        return (ret == 0) ? 0 : -1;
}




static gnutls_x509_crt *create_certificate_table(gnutls_datum *src)
{
        int ret;
        char *ptr, *start;
        size_t i = 1, nlen;
        gnutls_datum data;
        gnutls_x509_crt *crt_tbl = NULL;

        data.data = src->data;
        data.size = src->size;
        
        while ( (ptr = prelude_strnstr(data.data, BEGIN_CERT_STR, data.size)) ) {

                start = ptr;
                ptr += sizeof(BEGIN_CERT_STR) - 1;
                nlen = data.size - (ptr - (char *) data.data);
                
                if ( ! prelude_strnstr(ptr, END_CERT_STR, MIN(nlen, END_WITHIN_LEN)) ) {
                        data.size = nlen;
                        data.data = ptr;
                        continue;
                }
                
                crt_tbl = prelude_realloc(crt_tbl, ++i * sizeof(void *));
                if ( ! crt_tbl ) {
                        log(LOG_ERR, "memory exhausted.\n");
                        return NULL;
                }

                crt_tbl[i - 1] = NULL;
                gnutls_x509_crt_init(&crt_tbl[i - 2]);
                
                data.data = start;
                
                ret = gnutls_x509_crt_import(crt_tbl[i - 2], &data, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        log(LOG_ERR, "error importing certificate: %s.\n", gnutls_strerror(ret));
                        free(crt_tbl);
                        return NULL;
                }

                data.size = nlen;
                data.data = ptr;
        }

        return crt_tbl;
}




int tls_load_file(const char *filename, gnutls_datum *data)
{
        int ret, fd;
        struct stat st;

        fd = open(filename, O_RDONLY);
        if ( fd < 0 ) {
                log(LOG_ERR, "could not open %s for reading.\n", filename);
                return -1;
        }
        
        ret = fstat(fd, &st);
        if ( ret < 0 ) {
                log(LOG_ERR, "could not stat fd.\n");
                close(fd);
                return -1;
        }
        
        data->data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if ( ! data->data ) {
                log(LOG_ERR, "error mapping file to memory.\n");
                close(fd);
                return -1;
        }

        close(fd);
        data->size = st.st_size;
        
        return 0;
}



void tls_unload_file(gnutls_datum *data)
{
        munmap(data->data, data->size);
}




gnutls_x509_crt *tls_certificates_load(const char *filename, gnutls_x509_privkey *key)
{
        int ret;
        gnutls_datum data;
        gnutls_x509_crt *crt_tbl;
        
        ret = tls_load_file(filename, &data);
        if ( ret < 0 )
                return NULL;
        
        gnutls_x509_privkey_init(key);
        gnutls_x509_privkey_import(*key, &data, GNUTLS_X509_FMT_PEM);
        
        crt_tbl = create_certificate_table(&data);
        tls_unload_file(&data);

        if ( ! crt_tbl )
                gnutls_x509_privkey_deinit(*key);
        
        return crt_tbl;
}




gnutls_x509_crt *tls_certificates_search(gnutls_x509_crt *tbl, const char *dn, size_t size)
{
        int i, ret;

        for ( i = 0; tbl[i] != NULL; i++ ) {
                
                ret = cmp_certificate_issuer(tbl[i], dn, size);
                if ( ret == 0 )
                        return &tbl[i];
        }

        return NULL;
}


        
