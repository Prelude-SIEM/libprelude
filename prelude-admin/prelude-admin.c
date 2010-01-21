                        /*****
*
* Copyright (C) 2001, 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
* This file is part of the Prelude library.
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

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <netdb.h>

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
# include <pwd.h>
# include <grp.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <errno.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/extra.h>

#include "common.h"
#include "config-engine.h"
#include "prelude.h"
#include "idmef-message-print.h"

#include "server.h"
#include "tls-register.h"


#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
# define chown(x, y, z) (0)
# define fchown(x, y, z) (0)
# define getuid(x) (0)
# define getgid(x) (0)
#endif


struct rm_dir_s {
        prelude_list_t list;
        char *filename;
};


struct cmdtbl {
        char *cmd;
        int argnum;
        int (*cmd_func)(int argc, char **argv);
        void (*help_func)(void);
};


static const char *myprogname;
static unsigned int port = 5553;
static PRELUDE_LIST(rm_dir_list);
static prelude_option_t *parentopt;
static prelude_client_profile_t *profile;
static char *addr = NULL, *one_shot_passwd = NULL, *arg_command = NULL;
static prelude_bool_t gid_set = FALSE, uid_set = FALSE, detailed_listing = FALSE;
static prelude_bool_t prompt_passwd = FALSE, server_keepalive = FALSE, pass_from_stdin = FALSE;


int generated_key_size = 0;
prelude_bool_t server_confirm = TRUE;
int authority_certificate_lifetime = 0;
int generated_certificate_lifetime = 0;
static int64_t offset = -1, count = -1;


static const char *get_tls_config(void)
{
        static char buf[PATH_MAX];
        static int initialized = 0;

        if ( initialized )
                return buf;

        prelude_client_profile_get_default_config_dirname(NULL, buf, sizeof(buf));
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "/tls.conf");
        initialized = 1;

        return buf;
}


static int chown_cb(const char *filename, const struct stat *st, int flag)
{
        int ret;
        prelude_uid_t uid;
        prelude_gid_t gid;

        uid = uid_set ? prelude_client_profile_get_uid(profile) : (prelude_uid_t) -1;
        gid = gid_set ? prelude_client_profile_get_gid(profile) : (prelude_gid_t) -1;

        ret = chown(filename, uid, gid);
        if ( ret < 0 )
                fprintf(stderr, "could not change '%s' to UID:%d GID:%d: %s.\n\n", filename, (int) uid, (int) gid, strerror(errno));

        return ret;
}


static int do_chown(const char *name)
{
        int ret;
        char dirname[PATH_MAX];

        prelude_client_profile_get_profile_dirname(profile, dirname, sizeof(dirname));
        ret = ftw(dirname, chown_cb, 10);
        if ( ret < 0 )
                return -1;

        prelude_client_profile_get_backup_dirname(profile, dirname, sizeof(dirname));
        ret = ftw(dirname, chown_cb, 10);
        if ( ret < 0 )
                return -1;

        fprintf(stderr, "Changed '%s' ownership to UID:%d GID:%d.\n", name,
                uid_set ? (int) prelude_client_profile_get_uid(profile) : -1,
                gid_set ? (int) prelude_client_profile_get_gid(profile) : -1);

        return 0;
}



static void permission_warning(void)
{
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        fprintf(stderr,
"* WARNING: no --uid or --gid command line options were provided.\n*\n"

"* The profile will be created under the current UID (%d) and GID (%d). The\n"
"* created profile should be available for writing to the program that will\n"
"* be using it.\n*\n"

"* Your sensor WILL NOT START without sufficient permission to load the profile.\n"
"* [Please press enter if this is what you intend to do]\n",
                (int) prelude_client_profile_get_uid(profile), (int) prelude_client_profile_get_gid(profile));

        while ( getchar() != '\n' );
#endif
}




static int change_permission(int exist_uid, int exist_gid)
{
        fprintf(stderr,
"* WARNING: A profile named '%s' already exist with permission UID:%d, GID:%d.\n"
"* If you continue, '%s' permission will be updated to UID:%d, GID:%d.\n"
"* [Please press enter if this is what you intend to do]\n",
                prelude_client_profile_get_name(profile), exist_uid, exist_gid,
                prelude_client_profile_get_name(profile),
                (int) prelude_client_profile_get_uid(profile), (int) prelude_client_profile_get_gid(profile));

        while ( getchar() != '\n' );

        return do_chown(prelude_client_profile_get_name(profile));
}



static const char *lifetime_to_str(char *out, size_t size, int lifetime)
{
        if ( ! lifetime )
                snprintf(out, size, "unlimited");
        else
                snprintf(out, size, "%d days", lifetime);

        return out;
}


static void print_tls_settings(void)
{
        char buf1[128], buf2[128];

        lifetime_to_str(buf1, sizeof(buf1), generated_certificate_lifetime);
        lifetime_to_str(buf2, sizeof(buf2), authority_certificate_lifetime);

fprintf(stderr,

"TLS Options (from: %s):\n"
"  --key-len=LEN           : Profile private key length (default: %d bits).\n"
"  --cert-lifetime=DAYS    : Profile certificate lifetime (default: %s).\n"
"  --ca-cert-lifetime=DAYS : Authority certificate lifetime (default: %s).\n"

"\n", get_tls_config(), generated_key_size, buf1, buf2);
}


static void print_delete_help(void)
{
        fprintf(stderr, "Usage  : delete <profile>\n");
        fprintf(stderr, "Example: delete my-profile\n\n");

        fprintf(stderr, "Delete an analyzer profile.\n\n");
}


static void print_rename_help(void)
{
        fprintf(stderr,
"Usage  : rename <source profile> <target profile>\n"
"Example: rename prelude-lml-old prelude-lml-new\n\n"

"Rename an analyzer profile.\n\n");
}



static void print_registration_server_help(void)
{
        fprintf(stderr,
"Usage  : registration-server <Prelude-Manager profile> [options]\n"
"Example: registration-server prelude-manager\n\n"

"Launch a registration server for the specified Prelude-Manager profile. The\n"
"profile will be created if it does not exist. Registered analyzers will be able\n"
"to communicate with Prelude-Manager instance using this profile.\n\n"

"Options:\n"
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
"  --uid=UID            : UID or user used to create the analyzer profile.\n"
"  --gid=GID            : GID or group used to create the analyzer profile.\n"
#endif
"  --prompt             : Prompt for a password instead of auto generating it.\n"
"  --passwd=PASSWD      : Use provided password instead of auto generating it.\n"
"  --passwd-file=-|FILE : Read password from file instead of auto generating it (- for stdin).\n"
"  --keepalive          : Register analyzer in an infinite loop.\n"
"  --no-confirm         : Do not ask for confirmation on sensor registration.\n"
"  --listen             : Address to listen on for registration request (default is any:5553).\n\n");

        print_tls_settings();
}



static void print_register_help(void)
{
        fprintf(stderr,
"Usage  : register <analyzer profile> <wanted permission> <registration-server address> [options]\n"
"Example: register prelude-lml \"idmef:w\" 192.168.0.1\n\n"

"This command will register the specified analyzer profile to the remote\n"
"registration server. The analyzer profile will be created if it does not exist.\n\n"

"Options:\n"
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
"  --uid=UID             : UID or user used to create analyzer profile.\n"
"  --gid=GID             : GID or group used to create analyzer profile.\n"
#endif
"  --passwd=PASSWD       : Use provided password instead of prompting it.\n"
"  --passwd-file=-|FILE\t: Read password from file (- for stdin).\n\n");

        print_tls_settings();
}



static void print_add_help(void)
{
        fprintf(stderr,
"Usage  : add <analyzer profile>\n"
"Example: add prelude-lml\n\n"

"This command will create the specified analyzer profile.\n\n"

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
"Options:\n"
"  --uid=UID: UID or user used to create analyzer profile.\n"
"  --gid=GID: GID or group used to create analyzer profile.\n"
#endif

"\n");

        print_tls_settings();
}



static void print_chown_help(void)
{
        fprintf(stderr,
"Usage  : chown <analyzer profile> <--uid|--gid>\n"
"Example: chown prelude-lml --uid lmluser --gid lmlgroup\n\n"

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
"Options:\n"
"  --uid=UID : UID or user used as new profile permission.\n"
"  --gid=GID : GID to group used as new profile permission.\n"
#endif

"\n");
}



static void print_revoke_help(void)
{
        fprintf(stderr,
"Usage  : revoke <analyzer profile> <revoked analyzerID>\n"
"Example: revoke prelude-manager 227879253605921\n\n"

"This command will revoke the analyzer using the given analyzerID from the\n"
"specified profile. Analyzer using the revoked analyzerID won't be able to\n"
"communicate with the profile it was revoked from anymore.\n\n");
}


static void print_list_help(void)
{
        fprintf(stderr,
"Usage  : list [-l]\n"
"Example: list -l\n\n"

"Print the list of available profile, their permissions, and the certificate\n"
"issuer analyzerID.\n\n"

"Options:\n"
"  -l --long : Print detailed listing (include uid/gid, profile analyzerID).\n"

"\n");
}



static void print_print_help(void)
{
        fprintf(stderr,
"Usage  : print <file>\n"
"Example: print /path/to/file1 /path/to/file2 /path/to/fileN\n\n"

"Print the messages within a Prelude IDMEF binary file (example: failover file)\n"
"to stdout using an human readable format.\n\n"

"Options:\n"
"  --offset=OFFSET : Skip processing until 'offset' events.\n"
"  --count=COUNT   : Process at most 'count' events.\n"

"\n");
}


static void print_send_help(void)
{
        fprintf(stderr,
"Usage  : send <analyzer profile> <Prelude-Manager address> <file>\n"
"Example: send prelude-lml 192.168.0.1 /path/to/file1 /path/to/file2 /path/to/fileN\n\n"

"Send the messages within a Prelude IDMEF binary file (example: failover file)\n"
"to the specified Prelude-Manager address. The specified profile is used for\n"
"authentication.\n\n"

"Options:\n"
"  --offset=OFFSET : Skip processing until 'offset' events.\n"
"  --count=COUNT   : Process at most 'count' events.\n"
"\n");
}


#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
static int set_uid(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        prelude_uid_t uid;
        const char *p;
        struct passwd *pw;

        for ( p = optarg; isdigit((int) *p); p++ );

        if ( *p == 0 )
                uid = atoi(optarg);
        else {
                pw = getpwnam(optarg);
                if ( ! pw ) {
                        fprintf(stderr, "could not lookup user '%s'.\n", optarg);
                        return -1;
                }

                uid = pw->pw_uid;
        }

        uid_set = TRUE;
        prelude_client_profile_set_uid(profile, uid);

        return 0;
}



static int set_gid(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        prelude_gid_t gid;
        const char *p;
        struct group *grp;

        for ( p = optarg; isdigit((int) *p); p++ );

        if ( *p == 0 )
                gid = atoi(optarg);
        else {
                grp = getgrnam(optarg);
                if ( ! grp ) {
                        fprintf(stderr, "could not lookup group '%s'.\n", optarg);
                        return -1;
                }

                gid = grp->gr_gid;
        }

        gid_set = TRUE;
        prelude_client_profile_set_gid(profile, gid);

        return 0;
}
#endif


static int set_offset(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        offset = strtoll(optarg, NULL, 0);
        return 0;
}


static int set_count(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        count = strtoll(optarg, NULL, 0);
        return 0;
}



static int set_server_keepalive(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        server_keepalive = TRUE;
        return 0;
}



static int set_passwd(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        one_shot_passwd = strdup(optarg);
        return 0;
}


static int set_passwd_file(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        FILE *fd;
        size_t len;
        char buf[1024], *ptr;

        prompt_passwd = FALSE;

        if ( strcmp(optarg, "-") == 0 ) {
                fd = stdin;
                server_confirm = FALSE;
                pass_from_stdin = TRUE;
        } else {
                fd = fopen(optarg, "r");
                if ( ! fd ) {
                        fprintf(stderr, "could not open file '%s': %s\n", optarg, strerror(errno));
                        return -1;
                }
        }

        ptr = fgets(buf, sizeof(buf), fd);
        if ( fd != stdin )
                fclose(fd);

        if ( ! ptr )
                return -1;

        len = strlen(buf);
        if ( buf[len - 1] == '\n' )
                buf[len - 1] = 0;

        one_shot_passwd = strdup(buf);

        return 0;
}


static int set_prompt_passwd(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        prompt_passwd = TRUE;
        return 0;
}


static int set_server_no_confirm(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        server_confirm = FALSE;
        return 0;
}



static int set_server_listen_address(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;

        ret = prelude_parse_address(optarg, &addr, &port);
        if ( ret < 0 )
                fprintf(stderr, "could not parse address '%s'.\n", optarg);

        return ret;
}


static int set_key_len(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        generated_key_size = atoi(optarg);
        return 0;
}


static int set_cert_lifetime(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        generated_certificate_lifetime = atoi(optarg);
        return 0;
}


static int set_ca_cert_lifetime(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        authority_certificate_lifetime = atoi(optarg);
        return 0;
}


static int set_long_listing(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        detailed_listing = TRUE;
        return 0;
}


static void setup_permission_options(void)
{
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'u', "uid",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_uid, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'g', "gid",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_gid, NULL);
#endif
}


static void setup_read_options(void)
{
        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'o', "offset",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_offset, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'c', "count",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_count, NULL);
}


static void setup_list_options(void)
{
        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'l', "long",
                           NULL, PRELUDE_OPTION_ARGUMENT_NONE, set_long_listing, NULL);
}


static int read_tls_setting(void)
{
        int ret;
        char *ptr;
        config_t *cfg;
        unsigned int line;

        ret = _config_open(&cfg, get_tls_config());
        if ( ret < 0 ) {
                prelude_perror(ret, "could not open %s", get_tls_config());
                return -1;
        }

        line = 0;
        ptr = _config_get(cfg, NULL, "generated-key-size", &line);
        if ( ! ptr ) {
                fprintf(stderr, "%s: couldn't find \"generated-key-size\" setting.\n", get_tls_config());
                goto err;
        }

        generated_key_size = atoi(ptr);
        free(ptr);

        line = 0;
        ptr = _config_get(cfg, NULL, "authority-certificate-lifetime", &line);
        if ( ! ptr ) {
                fprintf(stderr, "%s: couldn't find \"authority-certificate-lifetime\" setting.\n", get_tls_config());
                goto err;
        }

        authority_certificate_lifetime = atoi(ptr);
        free(ptr);

        line = 0;
        ptr = _config_get(cfg, NULL, "generated-certificate-lifetime", &line);
        if ( ! ptr ) {
                fprintf(stderr, "%s: couldn't find \"generated-certificate-lifetime\" setting.\n", get_tls_config());
                goto err;
        }

        generated_certificate_lifetime = atoi(ptr);
        free(ptr);

  err:
        _config_close(cfg);

        return (ptr) ? 0 : -1;
}



static void setup_tls_options(void)
{
        int ret;

        ret = read_tls_setting();
        if ( ret < 0 )
                exit(1);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "key-len",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_key_len, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "cert-lifetime",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_cert_lifetime, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "ca-cert-lifetime",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_ca_cert_lifetime, NULL);
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



/*
 * If the client provided no configuration filename, it is important
 * that we use the default configuration (idmef-client.conf). Here,
 * we do a copy of this file to the analyzer profile directory, so
 * that admin request to the analyzer can be saved locally for this
 * analyzer.
 */
static int create_template_config_file(prelude_client_profile_t *profile)
{
        int ret;
        FILE *fd, *tfd;
        size_t len, wlen;
        char dirname[PATH_MAX], filename[PATH_MAX], buf[8192];

        prelude_client_profile_get_default_config_dirname(profile, dirname, sizeof(dirname));
        snprintf(dirname + strlen(dirname), sizeof(dirname) - strlen(dirname), "/idmef-client.conf");

        prelude_client_profile_get_config_filename(profile, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret == 0 )
                return 0;

        tfd = fopen(dirname, "r");
        if ( ! tfd ) {
                fprintf(stderr, "could not open '%s' for reading: %s.\n", dirname, strerror(errno));
                return -1;
        }

        fd = fopen(filename, "w");
        if ( ! fd ) {
                fclose(tfd);
                fprintf(stderr, "could not open '%s' for writing: %s.\n", filename, strerror(errno));
                return -1;
        }

        ret = fchown(fileno(fd), prelude_client_profile_get_uid(profile), prelude_client_profile_get_gid(profile));
        if ( ret < 0 )
                fprintf(stderr, "error changing '%s' ownership: %s.\n", filename, strerror(errno));

        while ( fgets(buf, sizeof(buf), tfd) ) {
                len = strlen(buf);

                wlen = fwrite(buf, 1, len, fd);
                if ( wlen != len && ferror(fd) ) {
                        fprintf(stderr, "error writing to '%s': %s.\n", filename, strerror(errno));
                        ret = -1;
                        goto err;
                }
        }

     err:
        fclose(fd);
        fclose(tfd);

        if ( ret < 0 )
                unlink(buf);

        return ret;
}


static int register_sensor_ident(const char *name, uint64_t *ident)
{
        FILE *fd;
        int ret, already_exist;
        char buf[256], filename[256];

        *ident = generate_analyzerid();

        prelude_client_profile_get_analyzerid_filename(profile, filename, sizeof(filename));

        already_exist = access(filename, F_OK);

        ret = open(filename, (already_exist == 0) ? O_RDONLY : O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP);
        if ( ret < 0 ) {
                fprintf(stderr, "error opening '%s': %s.\n", filename, strerror(errno));
                return -1;
        }

        fd = fdopen(ret, (already_exist == 0) ? "r" : "w");
        if ( ! fd ) {
                close(ret);
                fprintf(stderr, "error opening '%s': %s.\n", filename, strerror(errno));
                return -1;
        }

        if ( already_exist == 0 ) {
                if ( ! fgets(buf, sizeof(buf), fd) ) {
                        fclose(fd);
                        unlink(filename);
                        return register_sensor_ident(name, ident);
                }

                if ( ! sscanf(buf, "%" PRELUDE_SCNu64, ident) ) {
                        fclose(fd);
                        unlink(filename);
                        return register_sensor_ident(name, ident);
                }

                fclose(fd);
                return 0;
        }

        ret = fchown(fileno(fd), prelude_client_profile_get_uid(profile), prelude_client_profile_get_gid(profile));
        if ( ret < 0 )
                fprintf(stderr, "error changing '%s' ownership: %s.\n", filename, strerror(errno));

        fprintf(fd, "%" PRELUDE_PRIu64 "\n", *ident);
        fclose(fd);

        return 0;
}



static int anon_check_passwd(prelude_io_t *fd, char *passwd)
{
        int ret;
        ssize_t size;
        unsigned char *rbuf;

        size = prelude_io_write_delimited(fd, passwd, strlen(passwd) + 1);
        if ( size < 0 ) {
                fprintf(stderr, "error sending authentication token: %s.\n", prelude_strerror(size));
                return -1;
        }

        size = prelude_io_read_delimited(fd, &rbuf);
        if ( size < 2 ) {
                fprintf(stderr, "error receiving authentication result: %s.\n", prelude_strerror(size));
                return -1;
        }

        ret = memcmp(rbuf, "OK", 2);
        free(rbuf);

        if ( ret != 0 )
                return -1;

        return 0;
}


static inline gnutls_transport_ptr fd_to_ptr(int fd)
{
        union {
                gnutls_transport_ptr ptr;
                int fd;
        } data;

        data.fd = fd;

        return data.ptr;
}


static inline int ptr_to_fd(gnutls_transport_ptr ptr)
{
        union {
                gnutls_transport_ptr ptr;
                int fd;
        } data;

        data.ptr = ptr;

        return data.fd;
}


static ssize_t tls_pull(gnutls_transport_ptr fd, void *buf, size_t count)
{
        return read(ptr_to_fd(fd), buf, count);
}



static ssize_t tls_push(gnutls_transport_ptr fd, const void *buf, size_t count)
{
        return write(ptr_to_fd(fd), buf, count);
}


static gnutls_session new_tls_session(int sock, char *passwd)
{
        int ret;
        gnutls_session session;
        gnutls_anon_client_credentials anoncred;

        const int kx_priority[] = {
                GNUTLS_KX_ANON_DH,
#ifdef GNUTLS_SRP_ENABLED
                GNUTLS_KX_SRP, GNUTLS_KX_SRP_DSS, GNUTLS_KX_SRP_RSA,
#endif
                0
        };

        gnutls_init(&session, GNUTLS_CLIENT);
        gnutls_set_default_priority(session);
        gnutls_kx_set_priority(session, kx_priority);

#ifdef GNUTLS_SRP_ENABLED
        {
                gnutls_srp_client_credentials srpcred;
                gnutls_srp_allocate_client_credentials(&srpcred);
                gnutls_srp_set_client_credentials(srpcred, "prelude-adduser", passwd);
                gnutls_credentials_set(session, GNUTLS_CRD_SRP, srpcred);
        }
#endif

        gnutls_anon_allocate_client_credentials(&anoncred);
        gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

        gnutls_transport_set_ptr(session, fd_to_ptr(sock));
        gnutls_transport_set_pull_function(session, tls_pull);
        gnutls_transport_set_push_function(session, tls_push);

        ret = gnutls_handshake(session);
        if ( ret < 0 ) {
                const char *errstr;

                if (ret == GNUTLS_E_WARNING_ALERT_RECEIVED || ret == GNUTLS_E_FATAL_ALERT_RECEIVED)
                        errstr = gnutls_alert_get_name(gnutls_alert_get(session));
                else
                        errstr = gnutls_strerror(ret);

                fprintf(stderr, "\nGnuTLS handshake failed: %s.\n", errstr);

                return NULL;
        }

        return session;
}



static prelude_io_t *connect_manager(const char *addr, unsigned int port, char *passwd)
{
        int ret, sock;
        prelude_io_t *fd;
        gnutls_session session;
        char buf[sizeof("65535")];
        struct addrinfo hints, *ai;

        memset(&hints, 0, sizeof(hints));
        snprintf(buf, sizeof(buf), "%u", port);

#ifdef AI_ADDRCONFIG
        hints.ai_flags = AI_ADDRCONFIG;
#endif

        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        ret = getaddrinfo(addr, buf, &hints, &ai);
        if ( ret != 0 ) {
                fprintf(stderr, "could not resolve %s: %s.\n", addr,
                        (ret == EAI_SYSTEM) ? strerror(errno) : gai_strerror(ret));
                return NULL;
        }

        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if ( sock < 0) {
                fprintf(stderr, "error creating socket: %s.\n", strerror(errno));
                return NULL;
        }

        fprintf(stderr, "\nConnecting to registration server (%s:%u)... ", addr, port);
        fflush(stderr);

        ret = connect(sock, ai->ai_addr, ai->ai_addrlen);
        if ( ret < 0 ) {
                fprintf(stderr, "\ncould not connect to %s port %s: %s.\n", addr, buf, strerror(errno));
                close(sock);
                return NULL;
        }

        session = new_tls_session(sock, passwd);
        if ( ! session )
                return NULL;

        ret = prelude_io_new(&fd);
        if ( ret < 0 ) {
                fprintf(stderr, "\nerror creating an IO object.\n");
                gnutls_deinit(session);
                close(sock);
                return NULL;
        }

        prelude_io_set_tls_io(fd, session);

        if ( gnutls_auth_get_type(session) == GNUTLS_CRD_ANON && anon_check_passwd(fd, passwd) < 0 ) {
                fprintf(stderr, "Authentication failed.\n");
                return NULL;
        }

        fprintf(stderr, "Authentication succeeded.\n");

        return fd;
}



static int create_directory(prelude_client_profile_t *profile, const char *dirname, int flags)
{
        int ret;

        ret = mkdir(dirname, flags);
        if ( ret < 0 && errno != EEXIST ) {
                fprintf(stderr, "error creating directory %s: %s.\n", dirname, strerror(errno));
                return -1;
        }

        ret = chown(dirname, prelude_client_profile_get_uid(profile), prelude_client_profile_get_gid(profile));
        if ( ret < 0 ) {
                fprintf(stderr, "could not chown %s to %d:%d: %s.\n", dirname,
                        (int) prelude_client_profile_get_uid(profile),
                        (int) prelude_client_profile_get_gid(profile), strerror(errno));
                return -1;
        }

        return 0;
}



static int setup_analyzer_files(prelude_client_profile_t *profile, uint64_t *analyzerid,
                                gnutls_x509_privkey *key, gnutls_x509_crt *crt)
{
        int ret;
        char buf[256];
        const char *name;

        prelude_client_profile_get_profile_dirname(profile, buf, sizeof(buf));

        ret = create_directory(profile, buf, S_IRWXU|S_IRGRP|S_IXGRP);
        if ( ret < 0 ) {
                fprintf(stderr, "error creating directory %s: %s.\n", buf, strerror(errno));
                return -1;
        }

        *key = tls_load_privkey(profile);
        if ( ! *key )
                return -1;

        name = prelude_client_profile_get_name(profile);

        ret = create_template_config_file(profile);
        if ( ret < 0 )
                return -1;

        ret = register_sensor_ident(name, analyzerid);
        if ( ret < 0 )
                return -1;

        prelude_client_profile_set_analyzerid(profile, *analyzerid);

        prelude_client_profile_get_backup_dirname(profile, buf, sizeof(buf));
        return create_directory(profile, buf, S_IRWXU|S_IRWXG);
}



static int ask_one_shot_password(char **buf, const char *ask, ...)
{
        int ret;
        va_list ap;
        char *pass1, *pass2;
        char str[1024], askbuf[1024];

        va_start(ap, ask);
        vsnprintf(askbuf, sizeof(askbuf), ask, ap);
        va_end(ap);

        do
        {
                snprintf(str, sizeof(str), "Enter %s: ", askbuf);
                pass1 = getpass(str);
                if ( ! pass1 || ! (pass1 = strdup(pass1)) )
                        return -1;

                snprintf(str, sizeof(str), "Confirm %s: ", askbuf);
                pass2 = getpass(str);
                if ( ! pass2 ) {
                        memset(pass1, 0, strlen(pass1));
                        free(pass1);
                        return -1;
                }

                ret = strcmp(pass1, pass2);
                memset(pass2, 0, strlen(pass2));

                if ( ret == 0 ) {
                        *buf = pass1;
                        return 0;
                }

                memset(pass1, 0, strlen(pass1));
                free(pass1);
        } while ( TRUE );
}



static int add_to_rm_dir_list(const char *filename)
{
        struct rm_dir_s *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                fprintf(stderr, "memory exhausted.\n");
                return -1;
        }

        new->filename = strdup(filename);
        prelude_list_add(&rm_dir_list, &new->list);

        return 0;
}



static int flush_rm_dir_list(void)
{
        int ret;
        struct rm_dir_s *dir;
        prelude_list_t *tmp, *bkp;

        prelude_list_for_each_safe(&rm_dir_list, tmp, bkp) {
                dir = prelude_list_entry(tmp, struct rm_dir_s, list);

                ret = rmdir(dir->filename);
                if ( ret < 0 ) {
                        fprintf(stderr, "could not delete directory %s: %s.\n", dir->filename, strerror(errno));
                        return -1;
                }

                prelude_list_del(&dir->list);
                free(dir->filename);
                free(dir);
        }

        return 0;
}


static int del_cb(const char *filename, const struct stat *st, int flag)
{
        int ret;

        if ( flag != FTW_F )
                return (flag == FTW_DNR) ? -1 : add_to_rm_dir_list(filename);

        ret = unlink(filename);
        if ( ret < 0 )
                fprintf(stderr, "unlink %s: %s.\n", filename, strerror(errno));

        return ret;
}



static int delete_dir(const char *dirname)
{
        int ret;

        ret = ftw(dirname, del_cb, 10);
        if ( ret < 0 && errno != ENOENT ) {
                fprintf(stderr, "traversing '%s' returned an error: %s.\n", dirname, strerror(errno));
                return ret;
        }

        return flush_rm_dir_list();
}



static int delete_profile(prelude_client_profile_t *profile)
{
        int ret;
        char buf[PATH_MAX];

        prelude_client_profile_get_profile_dirname(profile, buf, sizeof(buf));
        if ( access(buf, F_OK) < 0 ) {
                fprintf(stderr, "Could not find profile '%s'.\n", prelude_client_profile_get_name(profile));
                return -1;
        }

        ret = delete_dir(buf);
        if ( ret < 0 )
                return ret;

        prelude_client_profile_get_backup_dirname(profile, buf, sizeof(buf));
        ret = delete_dir(buf);
        if ( ret < 0 )
                return ret;

        return 0;
}



static int rename_cmd(int argc, char **argv)
{
        int ret, i;
        prelude_string_t *err;
        const char *sname, *dname;
        char spath[256], dpath[256];
        prelude_client_profile_t *sprofile, *dprofile;

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - i != 2 )
                return -2;

        sname = argv[i];
        dname = argv[i + 1];

        if ( *sname == '\0' ) {
                fprintf(stderr, "Empty source profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        if ( *dname == '\0' ) {
                fprintf(stderr, "Empty target profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        ret = prelude_client_profile_new(&sprofile, sname);
        if ( ret < 0 ) {
                fprintf(stderr, "Error opening profile '%s': %s\n", sname, prelude_strerror(ret));
                return -1;
        }

        ret = prelude_client_profile_new(&sprofile, dname);
        if ( ret == 0 ) {
                fprintf(stderr, "Could not rename profile '%s' to '%s': profile '%s' already exist.\n", sname, dname, dname);
                return -1;
        }

        ret = _prelude_client_profile_new(&dprofile);
        prelude_client_profile_set_name(dprofile, dname);

        prelude_client_profile_get_profile_dirname(sprofile, spath, sizeof(spath));
        prelude_client_profile_get_profile_dirname(dprofile, dpath, sizeof(dpath));

        ret = rename(spath, dpath);
        if ( ret < 0 ) {
                fprintf(stderr, "Error renaming '%s' to '%s': %s.\n", spath, dpath, strerror(errno));
                return ret;
        }

        prelude_client_profile_get_backup_dirname(sprofile, spath, sizeof(spath));
        prelude_client_profile_get_backup_dirname(dprofile, dpath, sizeof(dpath));

        ret = rename(spath, dpath);
        if ( ret < 0 ) {
                fprintf(stderr, "error renaming '%s' to '%s': %s.\n", spath, dpath, strerror(errno));
                return ret;
        }

        fprintf(stderr, "Successfully renamed profile '%s' to '%s'.\n", sname, dname);

        return 0;
}



static int get_existing_profile_owner(const char *buf)
{
        int ret;
        struct stat st;

        ret = stat(buf, &st);
        if ( ret < 0 ) {
                if ( errno != ENOENT )
                        fprintf(stderr, "error stating %s: %s.\n", buf, strerror(errno));

                return -1;
        }

        if ( ! uid_set )
                prelude_client_profile_set_uid(profile, st.st_uid);

        if ( ! gid_set )
                prelude_client_profile_set_gid(profile, st.st_gid);

        if ( (uid_set && st.st_uid != prelude_client_profile_get_uid(profile)) ||
             (gid_set && st.st_gid != prelude_client_profile_get_gid(profile)) )
                return change_permission(st.st_uid, st.st_gid);

        uid_set = gid_set = TRUE;
        return 0;
}



static int add_analyzer(const char *name, uint64_t *analyzerid, gnutls_x509_privkey *key,
                        gnutls_x509_crt *crt, gnutls_x509_crt *ca_crt)
{
        int ret;
        char buf[PATH_MAX];

        prelude_client_profile_set_name(profile, name);
        prelude_client_profile_get_profile_dirname(profile, buf, sizeof(buf));

        ret = get_existing_profile_owner(buf);
        if ( ret < 0 ) {
                if ( errno != ENOENT )
                        return -1;

                if ( ! uid_set && ! gid_set )
                        permission_warning();

                if ( ! uid_set || ! gid_set ) {
                        if ( ! uid_set )
                                prelude_client_profile_set_uid(profile, getuid());

                        if ( ! gid_set )
                                prelude_client_profile_set_gid(profile, getgid());
                }
        }

        ret = setup_analyzer_files(profile, analyzerid, key, crt);
        if ( ret < 0 )
                return ret;

        ret = tls_load_ca_certificate(profile, *key, ca_crt);
        if ( ret < 0 )
                return ret;

        ret = tls_load_ca_signed_certificate(profile, *key, *ca_crt, crt);
        if ( ret < 0 )
                return ret;

        return 0;
}



static int add_cmd(int argc, char **argv)
{
        int ret, i;
        uint64_t analyzerid;
        prelude_string_t *err;
        gnutls_x509_privkey key;
        gnutls_x509_crt ca_crt, crt;
        prelude_client_profile_t *testprofile;

        ret = _prelude_client_profile_new(&profile);
        setup_permission_options();
        setup_tls_options();

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - ret != 1 )
                return -2;

        if ( *argv[i] == '\0' ) {
                fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        ret = prelude_client_profile_new(&testprofile, argv[i]);
        if ( ret == 0 ) {
                fprintf(stderr, "Could not create already existing profile '%s'.\n", argv[i]);
                return -1;
        }

        ret = add_analyzer(argv[i], &analyzerid, &key, &crt, &ca_crt);
        if ( ret < 0 )
                goto out;

        gnutls_x509_privkey_deinit(key);
        gnutls_x509_crt_deinit(crt);
        gnutls_x509_crt_deinit(ca_crt);

        fprintf(stderr, "Created profile '%s' with analyzerID '%" PRELUDE_PRIu64 "'.\n", argv[i], analyzerid);

out:
        if ( ret < 0 )
                delete_profile(profile);

        return ret;
}



static int chown_cmd(int argc, char **argv)
{
        int ret, i;
        prelude_string_t *err;

        _prelude_client_profile_new(&profile);

        setup_permission_options();
        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -2;
        }

        if ( argc - i < 1 )
                return -2;

        if ( *argv[i] == '\0' ) {
                fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        ret = prelude_client_profile_set_name(profile, argv[i]);
        if ( ret < 0 ) {
                fprintf(stderr, "Error loading analyzer profile '%s': %s.\n\n", argv[i], prelude_strerror(ret));
                return -1;
        }

        if ( ! uid_set && ! gid_set ) {
                fprintf(stderr, "Option --uid or --gid should be provided to change the profile permission.\n\n");
                return -1;
        }

        return do_chown(argv[i]);
}


static int del_cmd(int argc, char **argv)
{
        int ret, i;
        prelude_string_t *err;

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 )
                return ret;

        if ( argc - i <= 0 )
                return -2;

        ret = _prelude_client_profile_new(&profile);
        if ( ret < 0 ) {
                fprintf(stderr, "Error creating analyzer profile: %s.\n", prelude_strerror(ret));
                return -1;
        }

        for ( ; i < argc; i++ ) {
                if ( *argv[i] == '\0' ) {
                        fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                        return -1;
                }

                ret = prelude_client_profile_set_name(profile, argv[i]);
                if ( ret < 0 ) {
                        fprintf(stderr, "Error setting analyzer profile name: %s.\n", prelude_strerror(ret));
                        return -1;
                }

                ret = delete_profile(profile);
                if ( ret < 0 )
                        return ret;

                fprintf(stderr, "Successfully deleted analyzer profile '%s'.\n", argv[i]);
        }

        return 0;
}



static int register_cmd(int argc, char **argv)
{
        int ret, i;
        prelude_io_t *fd;
        uint64_t analyzerid;
        prelude_string_t *err;
        gnutls_x509_privkey key;
        gnutls_x509_crt crt, ca_crt;
        prelude_connection_permission_t permission_bits;

        ret = _prelude_client_profile_new(&profile);
        setup_permission_options();
        setup_tls_options();

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "passwd", NULL,
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_passwd, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "passwd-file", NULL,
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_passwd_file, NULL);

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - i < 3 )
                return -2;

        if ( *argv[i] == '\0' ) {
                fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        ret = prelude_connection_permission_new_from_string(&permission_bits, argv[i + 1]);
        if ( ret < 0 ) {
                fprintf(stderr, "could not parse permission: %s.\n", prelude_strerror(ret));
                return -1;
        }

        ret = prelude_parse_address(strdup(argv[i + 2]), &addr, &port);
        if ( ret < 0 ) {
                fprintf(stderr, "error parsing address '%s'.\n", argv[i + 3]);
                return -1;
        }

        ret = add_analyzer(argv[i], &analyzerid, &key, &crt, &ca_crt);
        if ( ret < 0 )
                return -1;

        fprintf(stderr,
                "\nYou now need to start \"%s\" registration-server on %s:\n"
                "example: \"%s registration-server prelude-manager\"\n\n",
                myprogname, argv[i + 2], myprogname);

        if ( ! one_shot_passwd ) {

                ret = ask_one_shot_password(&one_shot_passwd, "the one-shot password provided on %s", argv[i + 2]);
                if ( ret < 0 )
                        return -1;
        }

        fd = connect_manager(addr, port, one_shot_passwd);
        memset(one_shot_passwd, 0, strlen(one_shot_passwd));
        if ( ! fd )
                return -1;

        ret = tls_request_certificate(profile, fd, key, permission_bits);

        prelude_io_close(fd);
        prelude_io_destroy(fd);

        if ( ret < 0 )
                return -1;

        fprintf(stderr, "Successful registration to %s:%u.\n\n", addr, port);
        return 0;
}


static int generate_one_shot_password(char **buf)
{
        int i;
        char c, *mybuf;
        struct timeval tv;
        const int passlen = 8;
        const char letters[] = "01234567890abcdefghijklmnopqrstuvwxyz";

        gettimeofday(&tv, NULL);

        srand((unsigned int) getpid() * tv.tv_usec);

        mybuf = malloc(passlen + 1);
        if ( ! mybuf )
                return -1;

        for ( i = 0; i < passlen; i++ ) {
                c = letters[rand() % (sizeof(letters) - 1)];
                mybuf[i] = c;
        }

        mybuf[passlen] = '\0';

        *buf = mybuf;

        fprintf(stderr,
                "The \"%s\" password will be requested by \"%s register\"\n"
                "in order to connect. Please remove the quotes before using it.\n\n", mybuf, myprogname);

        return 0;
}




static int registration_server_cmd(int argc, char **argv)
{
        int ret, i;
        uint64_t analyzerid;
        prelude_string_t *err;
        gnutls_x509_privkey key;
        gnutls_x509_crt ca_crt, crt;

        ret = _prelude_client_profile_new(&profile);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'k', "keepalive", NULL,
                           PRELUDE_OPTION_ARGUMENT_NONE, set_server_keepalive, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "passwd", NULL,
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_passwd, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 0, "passwd-file", NULL,
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_passwd_file, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'p', "prompt", NULL,
                           PRELUDE_OPTION_ARGUMENT_NONE, set_prompt_passwd, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'n', "no-confirm", NULL,
                           PRELUDE_OPTION_ARGUMENT_NONE, set_server_no_confirm, NULL);

        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'l', "listen", NULL,
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_server_listen_address, NULL);

        setup_permission_options();
        setup_tls_options();

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - i < 1 )
                return -2;

        if ( *argv[i] == '\0' ) {
                fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        if ( pass_from_stdin )
                fprintf(stderr, "Warning: registration confirmation disabled as a result of reading from a pipe.\n\n");

        if ( prompt_passwd && one_shot_passwd ) {
                fprintf(stderr, "Options --prompt, --passwd, and --passwd-file are incompatible.\n\n");
                return -1;
        }

        ret = add_analyzer(argv[i], &analyzerid, &key, &crt, &ca_crt);
        if ( ret < 0 )
                return -1;

        if ( prompt_passwd ) {
                fprintf(stderr,
                        "\n  Please enter registration one-shot password.\n"
                        "  This password will be requested by \"%s\" in order to connect.\n\n", myprogname);

                ret = ask_one_shot_password(&one_shot_passwd, "");
        }

        else if ( ! one_shot_passwd )
                ret = generate_one_shot_password(&one_shot_passwd);

        if ( ret < 0 )
                return -1;

        ret = server_create(profile, addr, port, server_keepalive, one_shot_passwd, key, ca_crt, crt);
        memset(one_shot_passwd, 0, strlen(one_shot_passwd));

        gnutls_x509_privkey_deinit(key);
        gnutls_x509_crt_deinit(crt);
        gnutls_x509_crt_deinit(ca_crt);

        return ret;
}



static int revoke_cmd(int argc, char **argv)
{
        int ret, i;
        char *eptr = NULL;
        uint64_t analyzerid;
        prelude_string_t *err;
        gnutls_x509_privkey key;
        gnutls_x509_crt ca_crt, crt;

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &err, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - i < 2 )
                return -2;

        if ( *argv[i] == '\0' ) {
                fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        ret = prelude_client_profile_new(&profile, argv[i]);
        if ( ret < 0 ) {
                fprintf(stderr, "Error opening profile '%s': %s.\n", argv[i], prelude_strerror(ret));
                return -1;
        }

        key = tls_load_privkey(profile);
        if ( ! key )
                return -1;

        ret = tls_load_ca_certificate(profile, key, &ca_crt);
        if ( ret < 0 )
                return -1;

        ret = tls_load_ca_signed_certificate(profile, key, ca_crt, &crt);
        if ( ret < 0 )
                return -1;

        analyzerid = strtoull(argv[i + 1], &eptr, 0);
        if ( eptr != argv[i + 1] + strlen(argv[i + 1]) ) {
                fprintf(stderr, "Invalid analyzerid: '%s'.\n", argv[i + 1]);
                return -1;
        }

        ret = tls_revoke_analyzer(profile, key, crt, analyzerid);
        if ( ret == 0 )
                fprintf(stderr, "AnalyzerID '%s' already revoked from profile '%s'.\n", argv[i + 1], argv[i]);

        else if ( ret == 1 )
                fprintf(stderr, "Successfully revoked analyzerID '%s' from profile '%s'.\n", argv[i + 1], argv[i]);

        return ret;
}




static int read_messages(const char *filename, prelude_io_t *io,
                         int (*process_cb)(idmef_message_t *msg, void *data), void *data)
{
        int ret, i = 0;
        prelude_msg_t *msg;
        idmef_message_t *idmef;

        do {
                msg = NULL;

                ret = prelude_msg_read(&msg, io);
                if ( ret < 0 ) {
                        if ( prelude_error_get_code(ret) == PRELUDE_ERROR_EOF )
                                return 0;

                        fprintf(stderr, "error reading message from '%s': %s.\n", filename, prelude_strerror(ret));
                        return -1;
                }

                if ( offset != -1 && i++ < offset )
                        continue;

                if ( count != -1 && count-- == 0 )
                        return 0;

                ret = idmef_message_new(&idmef);
                if ( ret < 0 ) {
                        fprintf(stderr, "error creating IDMEF root message: %s.\n", prelude_strerror(ret));
                        prelude_msg_destroy(msg);
                        return -1;
                }

                ret = idmef_message_read(idmef, msg);
                if ( ret < 0 ) {
                        fprintf(stderr, "error decoding message: %s.\n", prelude_strerror(ret));
                        prelude_msg_destroy(msg);
                        idmef_message_destroy(idmef);
                        return -1;
                }

                ret = process_cb(idmef, data);

                idmef_message_destroy(idmef);
                prelude_msg_destroy(msg);
        } while ( ret >= 0 );

        return ret;
}


static int print_cb(idmef_message_t *idmef, void *data)
{
        idmef_message_print(idmef, data);
        return 0;
}


static int print_cmd(int argc, char **argv)
{
        FILE *fd;
        int i, ret;
        prelude_string_t *str;
        prelude_io_t *io, *out;

        setup_read_options();
        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &str, NULL);

        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - i < 1 )
                return -2;

        ret = prelude_io_new(&io);
        if ( ret < 0 )
                return ret;

        ret = prelude_io_new(&out);
        if ( ret < 0 )
                return ret;

        prelude_io_set_file_io(out, stdout);

        for ( ; i < argc; i++ ) {
                fd = fopen(argv[i], "r");
                if ( ! fd ) {
                        fprintf(stderr, "Error opening '%s' for reading: %s.\n", argv[i], strerror(errno));
                        return -1;
                }

                prelude_io_set_file_io(io, fd);

                ret = read_messages(argv[i], io, print_cb, out);
                prelude_io_close(io);

                if ( ret < 0 )
                        break;
        }

        prelude_io_destroy(out);
        prelude_io_destroy(io);

        return ret;
}


static int send_cb(idmef_message_t *idmef, void *data)
{
        prelude_client_send_idmef(data, idmef);
        return 0;
}


static int send_cmd(int argc, char **argv)
{
        FILE *fd;
        int ret, i;
        prelude_io_t *io;
        prelude_string_t *str;
        prelude_client_t *client;
        prelude_connection_pool_t *pool;
        prelude_connection_pool_flags_t flags;

        setup_read_options();

        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &str, NULL);
        if ( ret < 0 ) {
                prelude_perror(ret, "Option error");
                return -1;
        }

        if ( argc - i < 3 )
                return -2;

        if ( *argv[i] == '\0' ) {
                fprintf(stderr, "Empty profile name provided: '%s'.\n", argv[i]);
                return -1;
        }

        ret = prelude_io_new(&io);
        if ( ret < 0 )
                return ret;

        ret = prelude_client_new(&client, argv[i]);
        if ( ret < 0 )
                return ret;

        flags = prelude_client_get_flags(client);
        flags &= ~PRELUDE_CLIENT_FLAGS_HEARTBEAT;
        prelude_client_set_flags(client, flags);

        pool = prelude_client_get_connection_pool(client);
        flags = prelude_connection_pool_get_flags(pool);

        ret = prelude_client_init(client);

        flags &= ~(PRELUDE_CONNECTION_POOL_FLAGS_RECONNECT|PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER);
        prelude_connection_pool_set_flags(pool, flags);
        prelude_connection_pool_set_connection_string(pool, argv[i + 1]);

        ret = prelude_client_start(client);
        if ( ret < 0 ) {
                fprintf(stderr, "Error starting prelude-client: %s.\n", prelude_strerror(ret));
                return ret;
        }

        for ( i += 2; i < argc; i++ ) {
                fd = fopen(argv[i], "r");
                if ( ! fd ) {
                        fprintf(stderr, "error opening '%s' for reading: %s.\n", argv[i], strerror(errno));
                        return -1;
                }

                prelude_io_set_file_io(io, fd);
                ret = read_messages(argv[i], io, send_cb, client);
                prelude_io_close(io);

                if ( ret < 0 )
                        break;
        }

        prelude_client_destroy(client, PRELUDE_CLIENT_EXIT_STATUS_SUCCESS);
        return ret;
}


static PRELUDE_LIST(print_list);

typedef struct {
        prelude_list_t list;
        unsigned int sub;
        char *name;
        char *uid;
        char *gid;
        char *analyzerid;
        char *permission;
        char *tanalyzerid;
} print_entry_t;


static void print_info(const char *info, unsigned int pad)
{
        unsigned int i;
        size_t len = strlen(info);

        printf("%s", info);

        if ( pad < len ) {
                if ( pad != 0 )
                        putchar(' ');
                return;
        }

        pad -= strlen(info);
        for ( i = 0; i < pad; i++ )
                putchar(' ');
}


static void print_add(unsigned int sub, const char *name, const char *uid, const char *gid,
                      const char *analyzerid, const char *perm, const char *tanalyzerid)
{
        print_entry_t *ent = malloc(sizeof(*ent));

        ent->sub = sub;
        ent->name = strdup(name);
        ent->uid = strdup(uid);
        ent->gid = strdup(gid);
        ent->analyzerid = strdup(analyzerid);
        ent->permission = strdup(perm);
        ent->tanalyzerid = strdup(tanalyzerid);

        prelude_list_add_tail(&print_list, &ent->list);
}


static void print_added(void)
{
        unsigned int i;
        print_entry_t *ent;
        prelude_list_t *tmp, *bkp;
        unsigned int mtotal = 0, mpartial = 0;
        unsigned int mlen_name = 0, mlen_uid = 0, mlen_gid = 0, mlen_analyzerid = 0, mlen_tanalyzerid = 0, mlen_perm = 0;

        prelude_list_for_each(&print_list, tmp) {
                ent = prelude_list_entry(tmp, print_entry_t, list);

                mlen_name = MAX(mlen_name, strlen(ent->name));
                mlen_uid = MAX(mlen_uid, strlen(ent->uid));
                mlen_gid = MAX(mlen_gid, strlen(ent->gid));
                mlen_analyzerid = MAX(mlen_analyzerid, strlen(ent->analyzerid));
                mlen_tanalyzerid = MAX(mlen_tanalyzerid, strlen(ent->tanalyzerid));
                mlen_perm = MAX(mlen_perm, strlen(ent->permission));
        }

        print_info("Profile", ++mlen_name);
        if ( detailed_listing ) {
                print_info("UID", ++mlen_uid);
                print_info("GID", ++mlen_gid);
                print_info("AnalyzerID", ++mlen_analyzerid);
                mtotal += mlen_uid + mlen_gid + mlen_analyzerid;
                mpartial = mlen_uid + mlen_gid + mlen_analyzerid;
        }

        print_info("Permission", ++mlen_perm);
        printf("Issuer AnalyzerID\n");

        mpartial += mlen_name;
        mtotal += mlen_name + mlen_perm + ++mlen_tanalyzerid;

        for ( i = 0; i < mtotal; i++)
                putchar('-');
        putchar('\n');

        prelude_list_for_each_safe(&print_list, tmp, bkp) {
                ent = prelude_list_entry(tmp, print_entry_t, list);

                if ( ent->sub > 0 ) {
                        print_info("", mpartial);
                } else {
                        print_info(ent->name, mlen_name);
                        if ( detailed_listing ) {
                                print_info(ent->uid, mlen_uid);
                                print_info(ent->gid, mlen_gid);
                                print_info(ent->analyzerid, mlen_analyzerid);
                        }
                }

                print_info(ent->permission, mlen_perm);
                printf("%s\n", ent->tanalyzerid);

                free(ent->name);
                free(ent->uid);
                free(ent->gid);
                free(ent->analyzerid);
                free(ent->permission);
                free(ent->tanalyzerid);

                prelude_list_del(&ent->list);
                free(ent);
        }
}


static int list_cmd(int argc, char **argv)
{
        FILE *fd;
        DIR *dir;
        size_t size;
        struct stat st;
        struct dirent *dh;
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        struct group *gr;
        struct passwd *pw;
#endif
        gnutls_datum data;
        prelude_string_t *str;
        unsigned int cert_max, i;
        int ret,  permission;
        gnutls_x509_crt certs[1024];
        char dirname[PATH_MAX];
        char buf[1024], analyzerid[128], uidbuf[128] = { 0 }, gidbuf[128] = { 0 };

        setup_list_options();
        i = ret = prelude_option_read(parentopt, NULL, &argc, argv, &str, NULL);

        prelude_client_profile_get_profile_dirname(NULL, dirname, sizeof(dirname));

        dir = opendir(dirname);
        if ( ! dir ) {
                fprintf(stderr, "could not open '%s': %s.\n", dirname, strerror(errno));
                return -1;
        }

        ret = prelude_string_new(&str);
        if ( ret < 0 ) {
                closedir(dir);
                return ret;
        }

        while ( (dh = readdir(dir)) ) {
                if ( strcmp(dh->d_name, ".") == 0 || strcmp(dh->d_name, "..") == 0 )
                        continue;

                snprintf(buf, sizeof(buf), "%s/%s", dirname, dh->d_name);

                ret = stat(buf, &st);
                if ( ret < 0 ) {
                        if ( errno != EACCES )
                                fprintf(stderr, "error stating '%s': %s.\n", buf, strerror(errno));
                        continue;
                }

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
                pw = getpwuid(st.st_uid);
                if ( ! pw )
                        snprintf(uidbuf, sizeof(uidbuf), "%d", (int) st.st_uid);
                else
                        snprintf(uidbuf, sizeof(uidbuf), "%s", pw->pw_name);

                gr = getgrgid(st.st_gid);
                if ( ! gr )
                        snprintf(gidbuf, sizeof(gidbuf), "%d", (int) st.st_gid);
                else
                        snprintf(gidbuf, sizeof(gidbuf), "%s", gr->gr_name);
#endif

                snprintf(buf, sizeof(buf), "%s/%s/analyzerid", dirname, dh->d_name);
                fd = fopen(buf, "r");
                if ( ! fd )
                        snprintf(analyzerid, sizeof(analyzerid), "n/a");
                else {
                        if ( (! fgets(analyzerid, sizeof(analyzerid), fd)) || (size = strlen(analyzerid)) <= 0 ) {
                                fprintf(stderr, "failed to read analyzerID.\n");
                                continue;
                        }

                        fclose(fd);
                        analyzerid[size - 1] = 0;
                }

                snprintf(buf, sizeof(buf), "%s/%s/client.keycrt", dirname, dh->d_name);
                ret = _prelude_load_file(buf, &data.data, &size);
                if ( ret < 0 ) {
                        print_add(0, dh->d_name, uidbuf, gidbuf, analyzerid, "n/a", "n/a");
                        continue;
                }

                data.size = (unsigned int) size;
                cert_max = sizeof(certs) / sizeof(*certs);
                ret = _prelude_tls_crt_list_import(certs, &cert_max, &data, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        fprintf(stderr, "error importing certificate listing: %s.\n", gnutls_strerror(ret));
                        continue;
                }

                for ( i = 0; i < cert_max; i++ ) {

                        size = sizeof(buf);
                        ret = gnutls_x509_crt_get_dn_by_oid(certs[i], GNUTLS_OID_X520_COMMON_NAME, 0, 0, buf, &size);
                        if ( ret < 0 ) {
                                fprintf(stderr, "error reading certificate CN: %s.\n", gnutls_strerror(ret));
                                continue;
                        }

                        ret = sscanf(buf, "%d", (int *) &permission);
                        if ( ret != 1 )
                                continue;

                        size = sizeof(buf);
                        ret = gnutls_x509_crt_get_issuer_dn_by_oid(certs[i], GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, buf, &size);
                        if ( ret < 0 ) {
                                fprintf(stderr, "error reading certificate DN: %s.\n", gnutls_strerror(ret));
                                continue;
                        }

                        prelude_connection_permission_to_string(permission, str);
                        print_add(i, dh->d_name, uidbuf, gidbuf, analyzerid, prelude_string_get_string(str), buf);
                        prelude_string_clear(str);

                        gnutls_x509_crt_deinit(certs[i]);
                }
        }

        closedir(dir);
        prelude_string_destroy(str);

        print_added();

        return 0;
}


static const struct cmdtbl tbl[] = {
        { "add", 1, add_cmd, print_add_help                                                 },
        { "chown", 1, chown_cmd, print_chown_help                                           },
        { "del", 1, del_cmd, print_delete_help                                              },
        { "list", 0, list_cmd, print_list_help                                              },
        { "print", 1, print_cmd, print_print_help                                           },
        { "rename", 2, rename_cmd, print_rename_help                                        },
        { "register", 3, register_cmd, print_register_help                                  },
        { "registration-server", 1, registration_server_cmd, print_registration_server_help },
        { "revoke", 2, revoke_cmd, print_revoke_help                                        },
        { "send", 3, send_cmd, print_send_help                                              },
};


static int print_help(void)
{
        unsigned int i;

        fprintf(stderr, "\nUsage %s <subcommand> [options] [args]\n", myprogname);
        fprintf(stderr, "Type \"%s <subcommand>\" for help on a specific subcommand.\n\n", myprogname);
        fprintf(stderr, "Available subcommands:\n");

        for ( i = 0; i < sizeof(tbl) / sizeof(*tbl); i++ )
                fprintf(stderr, " %s\n", tbl[i].cmd);

        exit(1);
}


static int print_detailed_help(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        unsigned int i;

        for ( i = 0; i < sizeof(tbl) / sizeof(*tbl); i++ ) {
                if ( strcmp(arg_command, tbl[i].cmd) == 0 ) {
                        tbl[i].help_func();
                        exit(0);
                }
        }

        print_help();
        exit(0);
}


int main(int argc, char **argv)
{
        int ret = -1;
        unsigned int i;
        const char *slash;

        slash = strrchr(argv[0], '/');
        myprogname = slash ? slash + 1 : argv[0];

        if ( argc < 2 )
                print_help();

        arg_command = argv[1];
        prelude_init(NULL, NULL);

        ret = prelude_option_new(NULL, &parentopt);
        prelude_option_add(parentopt, NULL, PRELUDE_OPTION_TYPE_CLI, 'h', "help",
                           NULL, PRELUDE_OPTION_ARGUMENT_NONE, print_detailed_help, NULL);

        ret = gnutls_global_init();
        if ( ret < 0 ) {
                fprintf(stderr, "TLS initialization failed: %s", gnutls_strerror(ret));
                return -1;
        }

        ret = -1;

#ifdef NEED_GNUTLS_EXTRA
        gnutls_global_init_extra();
#endif

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        signal(SIGPIPE, SIG_IGN);
#endif

        umask(S_IRWXO);

        for ( i = 0; i < sizeof(tbl) / sizeof(*tbl); i++ ) {
                if ( strcmp(tbl[i].cmd, argv[1]) != 0 )
                        continue;

                ret = tbl[i].cmd_func(argc - 1, &argv[1]);
                if ( ret < 0 ) {
                        if ( ret == -2 )
                                tbl[i].help_func();

                        return ret;
                }

                break;
        }

        gnutls_global_deinit();

        if ( i == sizeof(tbl) / sizeof(*tbl) )
                print_help();

        return ret;
}




