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

/*
 * can be:
 * -----BEGIN CERTIFICATE-----
 * -----BEGIN X509 CERTIFICATE-----
 */

#define BEGIN_STR       "-----BEGIN "
#define END_STR         "-----END "


 
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



static int load_individual_cert(FILE *fd, gnutls_certificate_credentials cred)
{
        size_t len;
        char buf[65535];
        gnutls_datum cert, key;
        int ret = -1, got_start = 0, got_key = 0;
        
        cert.data = buf;
        
        for ( cert.size = 0;
              cert.size < sizeof(buf) && fgets(buf + cert.size, sizeof(buf) - cert.size, fd);
              cert.size += len ) {

                len = strlen(buf + cert.size);
                
                if ( ! got_start && strstr(buf + cert.size, BEGIN_STR) ) {
                        got_start = 1;
                        
                        if ( ! got_key && strstr(buf + cert.size, "KEY") )
                                got_key = 1;
                        
                        continue;
                }

                if ( ! strstr(buf + cert.size, END_STR) ) 
                        continue;

                cert.size += len;
                
                if ( got_key ) {
                        key.data = strdup(buf);
                        key.size = cert.size;
                        got_key = cert.size = len = 0;
                        continue;
                }
                
                ret = gnutls_certificate_set_x509_key_mem(cred, &cert, &key, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        log(LOG_ERR, "error importing certificate: %s.\n", gnutls_strerror(ret));                        
                        goto out;
                }
                
                len = cert.size = 0;
        }

  out:
        if ( key.data )
                free(key.data);
        
        return ret;
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




int tls_certificates_load(const char *filename, gnutls_certificate_credentials cred)
{
        int ret;
        FILE *fd;

        fd = fopen(filename, "r");
        if ( ! fd ) {
                log(LOG_ERR, "error opening %s for reading.\n", filename);
                return -1;
        }
        
        ret = load_individual_cert(fd, cred);
        fclose(fd);
        
        return ret;
}
