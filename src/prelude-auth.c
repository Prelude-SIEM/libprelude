/*****
*
* Copyright (C) 2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "socket-op.h"
#include "prelude-auth.h"




/*
 * Replace \n by \0, as we don't want to take care of them.
 */
static int filter_string(char *string, size_t len) 
{
        int i, ret = -1;
        
        for ( i = 0; i < len; i++ )
                if ( string[i] == '\n' ) {
                        string[i] = 0;
                        ret = 0;
                }

        return ret;
}



/*
 * Separate user and pass from a given string.
 */
static int separate_string(const char *string, size_t slen, char **user, char **pass) 
{
        int len;
        char *p = NULL;
        
        do {
                if ( ! *user && (p = strstr(string, "user ")) ) 
                        *user = strdup(p + 5);
                
                if ( ! *pass && (p = strstr(string, "pass ")) ) 
                        *pass = strdup(p + 5);

                if ( ! *user && ! *pass )
                        return -1;

                len = strlen(string) + 1;
                string += len;
                
        } while ((slen -= len) != 0);

        return 0;
}




/*
 * Parse the line 'line' comming from the authentication file,
 * and store the username and password in 'user' and 'pass'
 * passed arguments.
 */
static int parse_auth_line(char *line, char **addr, char **user, char **pass) 
{
        char *tmp;
        
        tmp = strtok(line, ":");
        if ( ! tmp ) {
                log(LOG_ERR, "malformed auth file.\n");
                return -1;
        }

        *addr = strdup(tmp);
        if (! *addr ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                return -1;
        }

        tmp = strtok(NULL, ":");
        if ( ! tmp ) {
                log(LOG_ERR, "malformed auth file.\n");
                return -1;
        }
        
        *user = strdup(tmp);
        if (! *user ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                return -1;
        }
        
        tmp = strtok(NULL, ":");
        if ( ! tmp ) {
                free(*user);
                log(LOG_ERR, "malformed auth file.\n");
                return -1;
        }

        *pass = strdup(tmp);
        if (! *pass ) {
                free(*user);
                log(LOG_ERR, "couldn't duplicate string.\n");
                return -1;
        }

        return 0;
}



/*
 * Read the next entry in the authentication file pointed by fd.
 */
static int auth_read_entry(FILE *fd, int *line, char **addr, char **user, char **pass) 
{
        int ret;
        char buf[1024];

        if ( fgets(buf, sizeof(buf), fd) ) {
                
                *line += 1;
                ret = parse_auth_line(buf, addr, user, pass);
                if ( ret < 0 ) {
                        log(LOG_ERR, "couldn't parse line %d.\n", *line);
                        return -1;
                }
                
                return 0;
        }
        
        return -1;
}



/*
 *
 */
static int cmp(const char *given_client, const char *client,
               const char *given_user, const char *user,
               const char *given_pass, const char *pass) 
{
        int ret;
        
        ret = strcmp(given_client, client);
        if ( ret != 0 )
                return -1;
        
        ret = strcmp(given_user, user);
        if ( ret != 0 )
                return -1;

        printf("cmp %s, %s\n", pass, given_pass);
        
        if ( strcmp(pass, given_pass) == 0 )
                return 0;
        else {
                log(LOG_INFO, "Invalid password for %s.\n", user);
                return -1;
        }

        return -1;
}



/*
 *
 */
static int check_account(const char *given_client, const char *given_user, const char *given_pass) 
{
        FILE *fd;
        char *client, *user, *pass;
        int line = 0, ret;
        
        fd = fopen(MANAGER_AUTH_FILE, "r");
        if ( ! fd ) {
                log(LOG_ERR, "couldn't open %s for reading.\n", MANAGER_AUTH_FILE);
                return -1;
        }

        while ( auth_read_entry(fd, &line, &client, &user, &pass) == 0 ) {                
                ret = cmp(given_client, client, given_user, user, given_pass, pass);
                
                free(client);
                free(user);
                free(pass);
                
                if (ret == 0) {
                        fclose(fd);
                        return 0;
                }
        }
        
        fclose(fd);

        return -1;
}



/*
 *
 */
static int get_account_infos(int sock, char **user, char **pass) 
{
        int tlen, ret, i = 1;       
        char *buf, *u = NULL, *p = NULL;

        do {
                tlen = socket_read_delimited(sock, (void **) &buf, read);                
                if ( tlen <= 0 ) {
                        if ( tlen < 0 )
                                log(LOG_ERR, "error reading socket.\n");
                        goto err;
                }

                ret = filter_string(buf, tlen);
                if ( ret < 0 ) {
                        log(LOG_ERR, "No string delimiter (\\n) found.\n");
                        goto err;
                }
                
                ret = separate_string(buf, tlen, &u, &p);
                if ( ret < 0 ) {
                        log(LOG_ERR, "there was an error parsing the command.\n");
                        goto err;
                }
                
        } while ( (u == NULL || p == NULL) && i-- );

        *user = u;
        *pass = p;
        
        return 0;

 err:
        if ( u ) free(u);
        if ( p ) free(p);

        return -1;
}





/*
 * Send authentication informations (user, pass) on socket.
 */
static int write_auth_infos(int sock, const char *user, const char *pass) 
{
        int ret, len;
        char buf[1024];
        
        len = snprintf(buf, sizeof(buf), "user %s\npass %s\n", user, pass);

        ret = socket_write_delimited(sock, buf, len, write);
        if ( ret < 0 ) {
                log(LOG_ERR,"error while writing authentication infos on socket.\n");
                return -1;
        }

        return 0;
}



/*
 * Read authentication result on our socket.
 */
static int read_auth_result(int sock) 
{
        int ret;
        char *buf;
        
        ret = socket_read_delimited(sock, (void **) &buf, read);
        if ( ret < 0 ) {
                log(LOG_ERR,"error while reading authentication result on socket.\n");
                return -1;
        }
        
        return strncmp(buf, "ok", sizeof(buf));
}



/*
 * Do the authentication procedure :
 * sending authentication informations and reading the result.
 */
static int do_auth(int sock, const char *user, const char *pass) 
{
        int ret;

        ret = write_auth_infos(sock, user, pass);
        if ( ret < 0 ) 
                return -1;

        ret = read_auth_result(sock);
        if ( ret < 0 ) 
                return -1;

        return 0;
}



/*
 * Open authentication filename in append mode,
 * with stream positionned at the end of the file,
 *
 * If the authentication file does not exist, it is
 * created with permission 600.
 */
static FILE *open_auth_file(const char *filename) 
{
        FILE *fd;
        int ret;
        
        ret = access(filename, F_OK);
        if ( ret < 0 && creat(filename, S_IRUSR|S_IWUSR) < 0 ) {
                log(LOG_ERR, "couldn't create %s.\n", filename);
                return NULL;
        }
        
        fd = fopen(filename, "a+");
        if ( ! fd ) {
                log(LOG_ERR, "couldn't open %s in append mode.\n", filename);
                return NULL;
        }

        return fd;
}




/*
 * Write new account (consisting of username 'user'
 * and password 'pass' to the authentication file.
 */
static int write_account(FILE *fd, const char *addr, const char *user, const char *pass) 
{
        int ret;
        
        ret = fseek(fd, 0, SEEK_END);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't seek to end of file.\n");
                return -1;
        }

        fwrite(addr, 1, strlen(addr), fd);
        fwrite(":", 1, 1, fd);
        fwrite(user, 1, strlen(user), fd);
        fwrite(":", 1, 1, fd);
        fwrite(pass, 1, strlen(pass), fd);
        fwrite(":\n", 1, 2, fd);

        return 0;
}




/*
 * Ask for an username, then return it.
 */
static char *ask_username(void) 
{
        char buf[1024];
        
        fprintf(stderr, "\nUsername to use to authenticate : ");
        
        fgets(buf, sizeof(buf), stdin);

        /* strip \n */
        buf[strlen(buf) - 1] = '\0';
        
        return strdup(buf);
}




/*
 * Ask for a password and ask to comfirm the entered one...
 * Will not return until success, will return a password.
 */
static char *ask_password(void) 
{
        char *comfirm, *pass;
        
        pass = strdup(getpass("Please enter a password for this user : "));
        if (! pass ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                return NULL;
        }
        
        comfirm = getpass("Please re-enter the password (comfirm) : ");
        if ( strcmp(pass, comfirm) != 0 ) {
                free(pass);
                fprintf(stderr, "Bad password, they don't match.\n");
                return ask_password();
        }

        return pass;
}



static char *ask_manager_address(void) 
{
        char buf[1024];
        
        fprintf(stderr, "\n\nWhat is the Manager address ? ");
        fgets(buf, sizeof(buf), stdin);

        /* strip \n */
        buf[strlen(buf) - 1] = '\0';
        
        return strdup(buf);
}




/*
 * Check if user 'user' already exist.
 */
static int account_already_exist(FILE *fd, const char *naddr, const char *nuser) 
{
        int line = 0;
        char *user, *pass, *addr;
        
        rewind(fd);
        while (auth_read_entry(fd, &line, &addr, &user, &pass) == 0 ) {

                if ( strcmp(naddr, addr) == 0 && strcmp(nuser, user) == 0 ) {
                        fprintf(stderr, "address %s with username %s already exist.\n", naddr, nuser);
                        return -1;
                }
        }

        return 0;
}




/*
 * Ask the user to confirm account for user 'user'
 * creation, by answering yes or no...
 *
 * return 0 if the account should be created, -1 otherwise.
 */
static int comfirm_account_creation(const char *user) 
{
        char buf[3];

        while ( buf[0] != 'y' && buf[0] != 'n' ) {
                fprintf(stderr, "Register user \"%s\" ? [y/n] : ", user);
                fgets(buf, sizeof(buf), stdin);
        }

        return (buf[0] == 'y') ? 0 : -1;
}




/*
 * Call subroutine in order to create an account,
 * and do all the necessary test to see if it is valid.
 */
static int ask_account_infos(FILE *fd, char **addr, char **user, char **pass) 
{
        int ret;

        *addr = ask_manager_address();
        if ( ! *addr ) {
                fclose(fd);
                return -1;
        }
        
        *user = ask_username();
        if ( ! *user ) {
                fclose(fd);
                free(*addr);
                return -1;
        }

        ret = account_already_exist(fd, *addr, *user);
        if ( ret < 0 ) {
                fclose(fd);
                free(*user);
                free(*addr);
                return -1;
        }
        
        *pass = ask_password();
        if ( ! *pass ) {
                free(*addr);
                free(*user);
                fclose(fd);
                return -1;
        }
        
        return 0;
}



/**
 * prelude_auth_create_account:
 * @filename: The filename to store account in.
 *
 * Ask for a new account creation which will be stored into 'filename'
 * which is the authentication file. crypted mean if the password will
 * be written in a crypted fashion or not.
 *
 * Returns: 0 on sucess, -1 otherwise
 */
int prelude_auth_create_account(const char *filename, const int crypted) 
{
        int ret;
        FILE *fd;
        char *addr, *user, *pass;

        fd = open_auth_file(filename);
        if ( ! fd ) 
                return -1;

        ret = ask_account_infos(fd, &addr, &user, &pass);
        if ( ret < 0 ) {
                fclose(fd);
                return -1;
        }
        
        ret = comfirm_account_creation(user);
        if ( ret == 0 ) 
                write_account(fd, addr, user, pass);

        free(user);
        free(pass);
        
        fclose(fd);

        return ret;
}




/**
 * prelude_auth_send:
 * @sock: Socket where authentication information should be sent.
 * @addr: Address of the server authentication is sent to.
 *
 * prelude_auth_send() get account information from @filename
 * corresponding to server @server, and send them to @sock.
 *
 * Returns: 0 if authentication was sucessful, -1 otherwise.
 */
int prelude_auth_send(int sock, const char *addr) 
{
        FILE *fd;
        int ret, line;
        char *user, *pass, *client;

        fd = fopen(SENSORS_AUTH_FILE, "r");
        if (! fd ) {
                log(LOG_ERR,"couldn't open authentication file %s.\n", SENSORS_AUTH_FILE);
                return -1;
        }

        ret = auth_read_entry(fd, &line, &client, &user, &pass);
        if ( ret < 0 )
                log(LOG_ERR,"couldn't read authentication file %s.\n", SENSORS_AUTH_FILE);
        else 
                ret = do_auth(sock, user, pass);
        
        fclose(fd);
        if (user) free(user);
        if (pass) free(pass);

        return ret;
}





/** 
 * prelude_auth_recv:
 * @sock: socket where authentication information should be read.
 * @addr: Address of the remote host to get login/password associated with.
 *
 * Authenticate the client sending authentication information on @sock.
 *
 * Returns: 0 on success, -1 if an error occured or authentication failed.
 */
int prelude_auth_recv(int sock, const char *addr) 
{
        int ret;
        char *user = NULL, *pass = NULL;
        
        ret = get_account_infos(sock, &user, &pass);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't read remote authentication informations.\n");
                return -1;
        }

        printf("%s, %s, %s\n", addr, user, pass);
        
        ret = check_account(addr, user, pass);
        if ( ret < 0 ) 
                socket_write_delimited(sock, "failed", 6, write);
        else
                socket_write_delimited(sock, "ok", 2, write);
        
        free(user);
        free(pass);
        
        return ret;
}
