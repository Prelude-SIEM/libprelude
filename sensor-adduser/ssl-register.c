#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>

#include <openssl/des.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

#include "ssl.h"
#include "ssl-register.h"
#include "ssl-gencrypto.h"
#include "prelude-io.h"
#include "ssl-registration-msg.h"


#define ACKMSGLEN ACKLENGTH + SHA_DIGEST_LENGTH + HEADLENGTH + PADMAXSIZE


static int resolve_addr(const char *hostname, struct in_addr *addr) 
{
        int ret;
        struct hostent *h;

        /*
         * This is not a hostname. No need to resolve.
         */
        ret = inet_aton(hostname, addr);
        if ( ret != 0 ) 
                return 0;
        
        h = gethostbyname(hostname);
        if ( ! h )
                return -1;

        assert(h->h_length >= sizeof(*addr));
        
        memcpy(addr, h->h_addr, h->h_length);
                
        return 0;
}



static int send_own_certificate(prelude_io_t *pio, des_key_schedule *skey1,
                                des_key_schedule *skey2, int expire, int keysize, int crypt_key) 
{
        int ret;
        X509 *x509ss;
        
	x509ss = prelude_ssl_gen_crypto(keysize, expire, SENSORS_KEY, crypt_key);
	if ( ! x509ss ) {
		fprintf(stderr, "\nRegistration failed\n");
		return -1;
	}

        ret = prelude_ssl_send_cert(pio, SENSORS_KEY, skey1, skey2);
        if ( ret < 0 ) {
                fprintf(stderr, "Error sending certificate.\n");
                return -1;
        }
        
        return 0;
}




static int recv_manager_certificate(prelude_io_t *pio, des_key_schedule *skey1, des_key_schedule *skey2)  
{
        uint16_t len;
        BUF_MEM ackbuf;
        int ret, certlen;
        unsigned char *buf;
        char cert[BUFMAXSIZE];
        
        len = prelude_io_read_delimited(pio, (void **)&buf);
        if ( len <= 0 ) {
                fprintf(stderr, "Error receiving registration message\n");
                return -1;
        }
        
	certlen = analyse_install_msg(buf, len, cert, len, skey1, skey2);
	if ( certlen < 0 ) {
		fprintf(stderr, "Bad message received - Registration failed.\n");
                return -1;
	}

	fprintf(stderr, "writing Prelude Manager certificate.\n");

	/*
         * save Manager certificate
         */
	if ( ! prelude_ssl_save_cert(MANAGERS_CERT, cert, certlen) ) {
		fprintf(stderr, "error writing Prelude-Report Certificate to %s\n", MANAGERS_CERT);
                return -1;
	}
        
        /*
         * send ack
         */
	ackbuf.length = ACKLENGTH;
	ackbuf.data = ACK;
	ackbuf.max = ACKLENGTH;

	len = build_install_msg(&ackbuf, buf, ACKMSGLEN, skey1, skey2);
	if ( len <= 0 ) {
		fprintf(stderr, "Error building message - Registration failed.\n");
                return -1;
	}
        
        ret = prelude_io_write_delimited(pio, buf, len);
        if ( ret < 0 ) {
                fprintf(stderr, "Error sending registration message.\n");
                return -1;
        }
        

        return 0;
}



static prelude_io_t *connect_server(const char *server, uint16_t port) 
{
        int sock, ret;
        prelude_io_t *pio;
        struct sockaddr_in sa;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if ( sock < 0 ) {
                fprintf(stderr, "couldn't open socket : %s.\n", strerror(errno));
                return NULL;
        }

        memset(&sa, '\0', sizeof(sa));
        
        ret = resolve_addr(server, &sa.sin_addr);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't resolve %s : %s.\n", server, strerror(errno));
                return NULL;
        }
        
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);

        ret = connect(sock, (struct sockaddr *) &sa, sizeof(sa));
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't connect : %s.\n", strerror(errno));
                return NULL;
        }

        pio = prelude_io_new();
        if ( ! pio ) {
                fprintf(stderr, "couldn't create a Prelude IO object : %s.\n", strerror(errno));
                return NULL;
        }

        prelude_io_set_socket_io(pio, sock);
        
        return pio;
}




static void ask_manager_addr(char **addr, uint16_t *port) 
{
        char buf[1024];
        
        fprintf(stderr, "\n\nWhat is the Manager address ? ");
        fgets(buf, sizeof(buf), stdin);

        buf[strlen(buf) - 1] = '\0';
        *addr = strdup(buf);

        fprintf(stderr, "What is the Manager port [5554] ? ");
        fgets(buf, sizeof(buf), stdin);
        *port = ( *buf == '\n' ) ? 5554 : atoi(buf);
}




static void ask_configuration(char **addr, uint16_t *port, int *keysize, int *expire, int *store_crypted) 
{
        int ret;
        char buf[1024];
        
        ask_manager_addr(addr, port);
        prelude_ssl_ask_settings(keysize, expire, store_crypted);
        
        if ( *expire )
                snprintf(buf, sizeof(buf), "%d days", *expire);
        else
                snprintf(buf, sizeof(buf), "Never");
        
        fprintf(stderr, "\n\n"
                "Manager address   : %s:%d\n"
                "Key length        : %d\n"
                "Expire            : %s\n"
                "Store key crypted : %s\n\n",
                *addr, *port, *keysize, buf, (*store_crypted) ? "Yes" : "No");

        
        while ( 1 ) {
                fprintf(stderr, "Is this okay [yes/no] : ");

                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf) - 1] = '\0';
                
                ret = strcmp(buf, "yes");
                if ( ret == 0 )
                        break;
                
                ret = strcmp(buf, "no");
                if ( ret == 0 )
                        ask_configuration(addr, port, keysize, expire, store_crypted);
        }
        
        fprintf(stderr, "\n");
}




int ssl_add_certificate(void)
{
	int ret;
        char *addr;
        uint16_t port;
        prelude_io_t *pio;
	char buf[BUFMAXSIZE];
	des_key_schedule skey1;
	des_key_schedule skey2;
        int keysize, expire, store_crypted;

        ask_configuration(&addr, &port, &keysize, &expire, &store_crypted);
        
	if ( des_generate_2key(&skey1, &skey2, 0) != 0 ) {
		fprintf(stderr, "Problem making one shot password - Registration Failed.\n");
                return -1;
	}
        
        fprintf(stderr,
                "Now please start \"manager-adduser\" on the Manager host.\n"
                "Press enter when done.\n");
        fgets(buf, sizeof(buf), stdin);

        
	fprintf(stderr, "connecting to Manager host (%s:%d)... ", addr, port);

        pio = connect_server(addr, port);
        if ( ! pio ) {
                fprintf(stderr, "error connecting to remote host - Registration failed.\n");
                return -1;
        }
        
	fprintf(stderr, "Connected.\n");

        ret = send_own_certificate(pio, &skey1, &skey2, expire, keysize, store_crypted);
        if ( ret < 0 ) {
                fprintf(stderr, "Error sending own certificate - Registration failed.\n");
                return -1;
        }

        ret = recv_manager_certificate(pio, &skey1, &skey2);
        if ( ret < 0 ) {
                fprintf(stderr, "Error receiving Manager certificate - Registration failed.\n");
                return -1;
        }
        
        prelude_io_close(pio);
        prelude_io_destroy(pio);

	return 0;
}


