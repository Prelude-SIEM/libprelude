/*****
*
* Copyright (C) 2001, 2002, 2003, 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
* All Rights Reserved
*
* This file is part of the Prelude program.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/extra.h>

#include "common.h"
#include "config-engine.h"
#include "prelude-client.h"
#include "prelude-getopt.h"

#include "server.h"
#include "tls-register.h"


#define TLS_CONFIG PRELUDE_CONFIG_DIR "/tls/tls.conf"


struct cmdtbl {
        char *cmd;
        int argnum;
        int (*cmd_func)(int argc, char **argv);
        void (*help_func)(void);
};


static uint16_t port = 5553;
static const char *addr = NULL;
static prelude_client_t *client;
static int gid_set = 0, uid_set = 0;
static int server_prompt_passwd = 0, server_keepalive = 0;


int generated_key_size = 0;
int authority_certificate_lifetime = 0;
int generated_certificate_lifetime = 0;



static void permission_warning(void)
{
        fprintf(stderr,
                "\n\n*** WARNING ***\n"
                "prelude-adduser was started without the --uid and --gid command line option.\n\n"
                
                "It means that analyzer specific files, that should be available for writing,\n"
                "will be created using the current UID (%d) and GID (%d).\n\n"
                
                "Your sensor won't start unless it is running under this UID or is a member of this GID.\n\n"
                "[Please press enter if this is what you plan to do]\n",
                prelude_client_get_uid(client), prelude_client_get_gid(client));
        
        while ( getchar() != '\n' );
}

        


static void print_delete_help(void)
{
        fprintf(stderr, "\ndel: Delete an existing analyzer.\n");
        fprintf(stderr, "usage: del <analyzer-name>\n\n");

        fprintf(stderr,
                "The delete command will remove the sensor files created through\n"
                "\"add\" command. Once this is done, the analyzer can't be used\n"
                "unless \"register\" or \"add\" is called again.\n");
}


static void print_rename_help(void)
{
        fprintf(stderr, "\nrename: Rename an existing analyzer.\n");
        fprintf(stderr, "usage: rename <analyzer-name> <analyzer-name>\n\n");
}



static void print_registration_server_help(void)
{
        fprintf(stderr, "\nregistration-server: Start the analyzer registration-server.\n");
        fprintf(stderr, "usage: registration-server <analyzer-name> [options]\n\n");

        fprintf(stderr,
                "Start a registration server that will be used to register sensors.\n"
                "This is used in order to register 'sending' analyzer to 'receiving'\n"
                "analyzer. <analyzer-name> should be set to the name of the 'receiving'\n"
                "analyzer, the one where 'sending' analyzer will register to.\n\n");
                
        fprintf(stderr, "Valid options:\n");

        fprintf(stderr, "\t--uid arg\t\t: UID to use to setup 'receiving' analyzer files.\n");
        fprintf(stderr, "\t--gid arg\t\t: GID to use to setup 'receiving' analyzer files.\n");
        fprintf(stderr, "\t--prompt\t\t: Prompt for a password instead of auto generating it.\n");
        fprintf(stderr, "\t--keepalive\t\t: Register analyzer in an infinite loop.\n");
}



static void print_register_help(void)
{
        fprintf(stderr, "register: Register an analyzer.\n");
        fprintf(stderr, "usage: register <analyzer-name> <registration-server address> [options]\n\n");

        fprintf(stderr,
                "Register both \"add\" the analyzer basic setup if needed\n"
                "and will also configure communication of this analyzer with a\n"
                "receiving analyzer (like a Manager) through the specified registration-server.\n\n");
        
        fprintf(stderr, "Valid options:\n");
        fprintf(stderr, "\t--uid arg\t\t: UID to use to setup analyzer files.\n");
        fprintf(stderr, "\t--gid arg\t\t: GID to use to setup analyzer files.\n");
}



static void print_add_help(void)
{
        fprintf(stderr, "add: Setup a new analyzer.\n");
        fprintf(stderr, "usage: add <analyzer-name> [options]\n\n");

        fprintf(stderr, "Valid options:\n");
        fprintf(stderr, "\t--uid arg\t\t: UID to use to setup analyzer files.\n");
        fprintf(stderr, "\t--gid arg\t\t: GID to use to setup analyzer files.\n");
}




static int set_uid(void **context, prelude_option_t *opt, const char *optarg) 
{
        uid_set = 1;
        prelude_client_set_uid(client, atoi(optarg));
        return prelude_option_success;
}



static int set_gid(void **context, prelude_option_t *opt, const char *optarg)
{
        gid_set = 1;
        prelude_client_set_gid(client, atoi(optarg));
        return prelude_option_success;
}



static int set_server_keepalive(void **context, prelude_option_t *opt, const char *optarg)
{
	server_keepalive = 1;
	return prelude_option_success;
}


static int set_server_prompt_passwd(void **context, prelude_option_t *opt, const char *optarg)
{
	server_prompt_passwd = 1;
	return prelude_option_success;
}



static uint64_t generate_analyzerid(void) 
{
        struct timeval tv;
        union {
                uint64_t val64;
                uint32_t val32[2];
        } combo;

        gettimeofday(&tv, NULL);

        combo.val32[0] = tv.tv_sec;
        combo.val32[1] = tv.tv_usec;
                
        return combo.val64;
}




static int register_sensor_ident(const char *name, uint64_t *ident) 
{
        FILE *fd;
        int ret, already_exist;
        char buf[256], filename[256];
        
        prelude_client_get_ident_filename(client, filename, sizeof(filename));

        already_exist = access(filename, F_OK);
        
        fd = fopen(filename, (already_exist == 0) ? "r" : "w");
        if ( ! fd ) {
                fprintf(stderr, "error opening %s: %s.\n", filename, strerror(errno));
                return -1;
        }
        
        ret = fchmod(fileno(fd), S_IRUSR|S_IWUSR|S_IRGRP);
        if ( ret < 0 ) 
                fprintf(stderr, "couldn't make ident file readable for all.\n");

        if ( already_exist == 0 ) {
                if ( ! fgets(buf, sizeof(buf), fd) ) {
                        fclose(fd);
                        unlink(filename);
                        return register_sensor_ident(name, ident);
                }
                
                if ( ! sscanf(buf, "%llu", ident) ) {
                        fclose(fd);
                        unlink(filename);
                        return register_sensor_ident(name, ident);
                }
                
                fprintf(stderr, "  - Using already allocated ident for %s: %llu.\n", name, *ident);
                
                return 0;
        }

        fprintf(fd, "%llu\n", *ident);
        fprintf(stderr, "  - Allocated ident for %s: %llu.\n", name, *ident);
        fclose(fd);
        
        return 0;
}



static gnutls_session new_tls_session(int sock, char *passwd)
{
        int ret;
        gnutls_session session;
        gnutls_srp_client_credentials cred;
        const int kx_priority[] = { GNUTLS_KX_SRP, GNUTLS_KX_SRP_DSS, GNUTLS_KX_SRP_RSA, 0 };
                
        gnutls_init(&session, GNUTLS_CLIENT);
        gnutls_set_default_priority(session);
        gnutls_kx_set_priority(session, kx_priority);
        
        gnutls_srp_allocate_client_credentials(&cred);
        gnutls_srp_set_client_credentials(cred, "prelude-adduser", passwd); 
        gnutls_credentials_set(session, GNUTLS_CRD_SRP, cred);

        gnutls_transport_set_ptr(session, (gnutls_transport_ptr) sock);
        
        ret = gnutls_handshake(session);
        if ( ret < 0 ) {
                const char *errstr;
                
                if (ret == GNUTLS_E_WARNING_ALERT_RECEIVED || ret == GNUTLS_E_FATAL_ALERT_RECEIVED)
                        errstr = gnutls_alert_get_name(gnutls_alert_get(session));
                else
                        errstr = gnutls_strerror(ret);
                        
                fprintf(stderr, "- GnuTLS handshake failed: %s.\n", errstr);
                
                return NULL;
        }
        
        return session;
}



static prelude_io_t *connect_manager(struct in_addr in, uint16_t port, char *passwd) 
{
        int ret, sock;
        prelude_io_t *fd;
        gnutls_session session;
        struct sockaddr_in daddr;
        
        daddr.sin_addr = in;
        daddr.sin_family = AF_INET;
        daddr.sin_port = htons(port);

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if ( sock < 0) {
                fprintf(stderr, "error creating socket.\n");
                return NULL;
        }

        ret = connect(sock, (struct sockaddr *) &daddr, sizeof(daddr));
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't connect to %s.\n", addr);
                close(sock);
                return NULL;
        }

        session = new_tls_session(sock, passwd);
        if ( ! session )
                return NULL;
        
        fd = prelude_io_new();
        if ( ! fd ) {
                fprintf(stderr, "error creating an IO object.\n");
                gnutls_deinit(session);
                close(sock);
                return NULL;
        }
                
        prelude_io_set_tls_io(fd, session);

        return fd;
}





static int ask_one_shot_password(char **buf) 
{
        int ret;
        char *pass, *confirm;
        
        pass = getpass("\n  - Enter registration one shot password: ");
        if ( ! pass )
                return -1;

        pass = strdup(pass);
        if ( ! pass )
                return -1;
        
        confirm = getpass("  - Please confirm one shot password: ");
        if ( ! confirm )
                return -1;

        ret = strcmp(pass, confirm);
        memset(confirm, 0, strlen(confirm));

        if ( ret == 0 ) {
                *buf = pass;
                return 0;
        }

        fprintf(stderr, "    - Bad password, they don't match.\n");
        
        memset(pass, 0, strlen(pass));
        free(pass);
        
        return ask_one_shot_password(buf);
}



static int setup_analyzer_files(prelude_client_t *client, uint64_t analyzerid,
                                gnutls_x509_privkey *key, gnutls_x509_crt *crt) 
{
        int ret;
        char buf[256];
        const char *name;
        
        name = prelude_client_get_name(client);
        ret = register_sensor_ident(name, &analyzerid);
        if ( ret < 0 )
                return -1;

        prelude_client_set_analyzerid(client, analyzerid);
        
        *key = tls_load_privkey(client);  
        if ( ! *key )
                return -1;
        
        prelude_client_get_backup_filename(client, buf, sizeof(buf));

        /*
         * The user may have changed permission, and we don't want
         * to be vulnerable to a symlink attack anyway. 
         */
        fprintf(stderr, "  - Creating %s...\n", buf);

        ret = mkdir(buf, S_IRWXU|S_IRWXG);
        if ( ret < 0 && errno != EEXIST ) {
                fprintf(stderr, "error creating directory %s: %s.\n", buf, strerror(errno));
                return -1;
        }

        ret = chown(buf, prelude_client_get_uid(client), prelude_client_get_gid(client));
        if ( ret < 0 ) {
                fprintf(stderr, "could not chown %s to %d:%d: %s.\n", buf,
                        prelude_client_get_uid(client), prelude_client_get_gid(client),
                        strerror(errno));
                return -1;
        }
        
        return 0;
}



static int rename_cmd(int argc, char **argv)
{
        const char *sname, *dname;
        char spath[256], dpath[256];
        prelude_client_t *sclient, *dclient;
        
        sname = argv[2];
        dname = argv[3];
        fprintf(stderr, "Renaming analyzer %s to %s\n", sname, dname);
        
        sclient = prelude_client_new(0);
        prelude_client_set_name(sclient, sname);
        
        dclient = prelude_client_new(0);
        prelude_client_set_name(dclient, dname);

        prelude_client_get_ident_filename(sclient, spath, sizeof(spath));
        prelude_client_get_ident_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);

        prelude_client_get_backup_filename(sclient, spath, sizeof(spath));
        prelude_client_get_backup_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);
        
        prelude_client_get_tls_key_filename(sclient, spath, sizeof(spath));
        prelude_client_get_tls_key_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);

        prelude_client_get_tls_client_keycert_filename(sclient, spath, sizeof(spath));
        prelude_client_get_tls_client_keycert_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);

        prelude_client_get_tls_server_keycert_filename(sclient, spath, sizeof(spath));
        prelude_client_get_tls_server_keycert_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);

        prelude_client_get_tls_client_trusted_cert_filename(sclient, spath, sizeof(spath));
        prelude_client_get_tls_client_trusted_cert_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);

        prelude_client_get_tls_server_trusted_cert_filename(sclient, spath, sizeof(spath));
        prelude_client_get_tls_server_trusted_cert_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);
        
        prelude_client_get_tls_server_ca_cert_filename(sclient, spath, sizeof(spath));
        prelude_client_get_tls_server_ca_cert_filename(dclient, dpath, sizeof(dpath));
        fprintf(stderr, "- renaming %s to %s.\n", spath, dpath);
        rename(spath, dpath);
        
        return prelude_option_success;
}



static int add_analyzer(const char *name, gnutls_x509_privkey *key, gnutls_x509_crt *crt)
{
        int ret;
        char buf[256];
        
        fprintf(stderr, "- Adding analyzer %s.\n", name);

        prelude_client_set_name(client, name);
        
        if ( ! uid_set || ! gid_set ) {
                if ( ! uid_set )
                        prelude_client_set_uid(client, getuid());

                if ( ! gid_set )
                        prelude_client_set_gid(client, getgid());

                prelude_client_get_backup_filename(client, buf, sizeof(buf));
                
                ret = access(buf, W_OK);
                if ( ret < 0 )
                        permission_warning();
        }

        return setup_analyzer_files(client, generate_analyzerid(), key, crt);
}



static int add_cmd(int argc, char **argv)
{
        int ret;
        prelude_option_t *opt;
        gnutls_x509_privkey key;

        client = prelude_client_new(0);
        
        opt = prelude_option_new(NULL);
        prelude_option_add(opt, CLI_HOOK, 'u', "uid", NULL, required_argument, set_uid, NULL);
        prelude_option_add(opt, CLI_HOOK, 'g', "gid", NULL, required_argument, set_gid, NULL);
        
        ret = prelude_option_parse_arguments(NULL, opt, NULL, argc - 2, &argv[2]);
        if ( ret < 0 )
                return -1;

        prelude_option_destroy(opt);

        return add_analyzer(argv[2], &key, NULL);
}



static int del_cmd(int argc, char **argv)
{
        char buf[512];

        client = prelude_client_new(0);
        if ( ! client )
                return -1;

        prelude_client_set_name(client, argv[2]);
        fprintf(stderr, "- Deleting analyzer %s\n", argv[2]);
        
        prelude_client_get_backup_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        prelude_client_get_ident_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);

        prelude_client_get_tls_key_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        prelude_client_get_tls_client_keycert_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        prelude_client_get_tls_server_keycert_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        prelude_client_get_tls_client_trusted_cert_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        prelude_client_get_tls_server_trusted_cert_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        prelude_client_get_tls_server_ca_cert_filename(client, buf, sizeof(buf));
        fprintf(stderr, "  - Removing %s...\n", buf);
        unlink(buf);
        
        return 0;
}



static int register_cmd(int argc, char **argv)
{
        int ret;
        char *pass;
        struct in_addr in;
        prelude_io_t *fd;
        prelude_option_t *opt;
        gnutls_x509_privkey key;
        char *ptr, *addr = strdup(argv[3]);

        client = prelude_client_new(0);

        opt = prelude_option_new(NULL);
        prelude_option_add(opt, CLI_HOOK, 'u', "uid", NULL, required_argument, set_uid, NULL);
        prelude_option_add(opt, CLI_HOOK, 'g', "gid", NULL, required_argument, set_gid, NULL);
        
        ret = prelude_option_parse_arguments(NULL, opt, NULL, argc - 3, &argv[3]);
        if ( ret < 0 )
                return -1;
        
        prelude_option_destroy(opt);
        
        ret = add_analyzer(argv[2], &key, NULL);
        if ( ret < 0 )
                return -1;

        fprintf(stderr, "\n- Registring analyzer %s to %s.\n\n", argv[2], argv[3]);
        
        fprintf(stderr,
                "  You now need to start \"prelude-adduser\" on the server host where\n"
                "  you need to register to:\n\n"

                "  use: \"prelude-adduser registration-server <analyzer-name>\"\n\n"
                
                "  Please remember that \"prelude-adduser\" should be used to register\n"
                "  every server used by this analyzer.\n\n");

        
        fprintf(stderr,
                "  Enter the one-shot password provided by the \"prelude-adduser\" program:\n");

        ret = ask_one_shot_password(&pass);
        if ( ret < 0 )
                return -1;

        ptr = strchr(addr, ':');
        if ( ptr ) {
                *ptr++ = '\0';
                port = atoi(ptr);
        }
        
	fprintf(stderr, "  - connecting to registration server (%s:%d)...\n", addr, port);
        
        ret = prelude_resolve_addr(addr, &in);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't resolve %s.\n", addr);
                return -1;
        }
        
        fd = connect_manager(in, port, pass);
        if ( ! fd ) 
                return -1;
        
        ret = tls_request_certificate(client, fd, key);
        if ( ret < 0 )
                return -1;

        fprintf(stderr, "\n- %s registration to %s successful.\n\n", argv[2], argv[3]);
        
        return 0;
}



static int registration_server_cmd(int argc, char **argv)
{
        int ret;
        prelude_option_t *opt;
        gnutls_x509_privkey key;
        gnutls_x509_crt ca_crt, crt;
        
        opt = prelude_option_new(NULL);
        client = prelude_client_new(0);
        
        prelude_option_add(opt, CLI_HOOK, 'k', "keepalive", NULL,
                           no_argument, set_server_keepalive, NULL);
		
	prelude_option_add(opt, CLI_HOOK, 'p', "prompt", NULL,
                           no_argument, set_server_prompt_passwd, NULL);
        
        prelude_option_add(opt, CLI_HOOK, 'u', "uid", NULL, required_argument, set_uid, NULL);
        prelude_option_add(opt, CLI_HOOK, 'g', "gid", NULL, required_argument, set_gid, NULL);
        
        ret = prelude_option_parse_arguments(NULL, opt, NULL, argc - 2, &argv[2]);
        if ( ret < 0 )
                return -1;
        
        prelude_option_destroy(opt);
        
        ret = add_analyzer(argv[2], &key, NULL);
        if ( ret < 0 )
                return -1;
        
        ret = tls_load_ca_certificate(client, key, &ca_crt);
        if ( ret < 0 )
                return -1;
        
        ret = tls_load_ca_signed_certificate(client, key, ca_crt, &crt);
        if ( ret < 0 )
                return -1;
        
        fprintf(stderr, "\n- Starting registration server.\n");
        return server_create(client, server_keepalive, server_prompt_passwd, key, ca_crt, crt);
}




static int print_help(struct cmdtbl *tbl)
{
        int i;
        
        fprintf(stderr, "Usage prelude-adduser <subcommand> [options] [args]\n");
        fprintf(stderr, "Type \"prelude-adduser help <subcommand>\" for help on a specific subcommand.\n\n");
        fprintf(stderr, "Available subcommands:\n");
        
        for ( i = 0; tbl[i].cmd; i++ )
                fprintf(stderr, " %s\n", tbl[i].cmd);
        
        exit(1);
}



static const char *lifetime_to_str(char *out, size_t size, int lifetime)
{
        if ( ! lifetime )
                snprintf(out, size, "unlimited");
        else
                snprintf(out, size, "%d days", lifetime);
        
        return out;
}



static int read_tls_setting(void)
{
        int line = 0;
        config_t *cfg;
        char buf[128];
        const char *ptr;
        
        cfg = config_open(TLS_CONFIG);
        if ( ! cfg ) {
                fprintf(stderr, "couldn't open %s.\n", TLS_CONFIG);
                return -1;
        }

        ptr = config_get(cfg, NULL, "generated-key-size", &line);
        if ( ! ptr ) {
                fprintf(stderr, "%s: couldn't find \"generated-key-size\" setting.\n", TLS_CONFIG);
                goto err;
        }
        
        generated_key_size = atoi(ptr);

        line=0;
        ptr = config_get(cfg, NULL, "authority-certificate-lifetime", &line);
        if ( ! ptr ) {
                fprintf(stderr, "%s: couldn't find \"authority-certificate-lifetime\" setting.\n", TLS_CONFIG);
                goto err;
        }
        
        authority_certificate_lifetime = atoi(ptr);

        ptr = config_get(cfg, NULL, "generated-certificate-lifetime", &line);
        if ( ! ptr ) {
                fprintf(stderr, "%s: couldn't find \"generated-certificate-lifetime\" setting.\n", TLS_CONFIG);
                goto err;
        }
        
        generated_certificate_lifetime = atoi(ptr);

        fprintf(stderr, "\n- Using default TLS settings from %s:\n", TLS_CONFIG);
        fprintf(stderr, "  - Generated key size: %d bits.\n", generated_key_size);
        
        fprintf(stderr, "  - Authority certificate lifetime: %s.\n",
                lifetime_to_str(buf, sizeof(buf), authority_certificate_lifetime));
        
        fprintf(stderr, "  - Generated certificate lifetime: %s.\n\n",
                lifetime_to_str(buf, sizeof(buf), authority_certificate_lifetime));

  err:        
        config_close(cfg);
        
        return (ptr) ? 0 : -1;
}



int main(int argc, char **argv) 
{
        int i, k, ret = -1;
        struct cmdtbl tbl[] = {
                { "add", 1, add_cmd, print_add_help                                                 },
                { "del", 1, del_cmd, print_delete_help                                              },
                { "rename", 2, rename_cmd, print_rename_help                                        },
                { "register", 2, register_cmd, print_register_help                                  },
                { "registration-server", 1, registration_server_cmd, print_registration_server_help },
                { NULL, 0, NULL, NULL },
        };

        if ( argc == 1 )
                print_help(tbl);
        
        for ( k = 0; k < argc; k++ )
                if ( *argv[k] == '-' )
                        break;

        ret = read_tls_setting();
        if ( ret < 0 )
                return -1;

        ret = -1;
        gnutls_global_init();
        gnutls_global_init_extra();
        
        for ( i = 0; tbl[i].cmd; i++ ) {
                if ( strcmp(tbl[i].cmd, argv[1]) != 0 )
                        continue;
                
                if ( k - 2 < tbl[i].argnum ) {
                        tbl[i].help_func();
                        exit(1);
                }
                
                ret = tbl[i].cmd_func(argc, argv);
                break;
        }
        
        gnutls_global_deinit();
        
        if ( ret < 0 )
                print_help(tbl);
        
        return ret;
}




