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
#include <inttypes.h>

#include "common.h"
#include "prelude-io.h"
#include "prelude-auth.h"





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
static int check_account(const char *authfile, const char *given_client,
                         const char *given_user, const char *given_pass) 
{
        FILE *fd;
        int line = 0, ret;
        char *client, *user, *pass;
        
        fd = fopen(authfile, "r");
        if ( ! fd ) {
                log(LOG_ERR, "couldn't open %s for reading.\n", authfile);
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
static int get_account_infos(prelude_io_t *fd, char **user, char **pass) 
{
        int ret;

        ret = prelude_io_read_delimited(fd, (void **) user);
        if ( ret <= 0 ) {
                if ( ret < 0 )
                        log(LOG_ERR, "error reading data.\n");
                goto err;
        }

        ret = prelude_io_read_delimited(fd, (void **) pass);
        if ( ret <= 0 ) {
                if ( ret < 0 )
                        log(LOG_ERR, "error reading data.\n");
                goto err;
        }
        
        return 0;

 err:
        if ( *user ) free(*user);
        if ( *pass ) free(*pass);

        return -1;
}





/*
 * Send authentication informations (user, pass).
 */
static int write_auth_infos(prelude_io_t *fd, const char *user, const char *pass) 
{
        int ret;

        ret = prelude_io_write_delimited(fd, user, strlen(user) + 1);
        if ( ret < 0 ) {
                log(LOG_ERR, "error writing username for authentication.\n");
                return -1;
        }

        ret = prelude_io_write_delimited(fd, pass, strlen(pass) + 1);
        if ( ret < 0 ) {
                log(LOG_ERR, "error writing password for authentication.\n");
                return -1;
        }

        return 0;
}



/*
 * Read authentication result.
 */
static int read_auth_result(prelude_io_t *fd) 
{
        int ret;
        char *buf;
        
        ret = prelude_io_read_delimited(fd, (void **) &buf);
        if ( ret <= 0 ) {
                log(LOG_ERR,"couldn't read authentication result.\n");
                return -1;
        }
        
        ret = strncmp(buf, "ok", sizeof(buf));
        free(buf);

        return ret;
}



/*
 * Do the authentication procedure :
 * sending authentication informations and reading the result.
 */
static int do_auth(prelude_io_t *fd, const char *user, const char *pass) 
{
        int ret;

        ret = write_auth_infos(fd, user, pass);
        if ( ret < 0 ) 
                return -1;

        ret = read_auth_result(fd);
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
int prelude_auth_create_account(const char *filename) 
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
        free(addr);
        
        fclose(fd);

        return ret;
}




/**
 * prelude_auth_send:
 * @authfile: Filename containing username/password pair.
 * @fd: Pointer on a #prelude_io_t object where authentication should occur.
 * @addr: Address of the server authentication is sent to.
 *
 * prelude_auth_send() get account information from @filename
 * corresponding to server @server, and send them to @sock.
 *
 * Returns: 0 if authentication was sucessful, -1 otherwise.
 */
int prelude_auth_send(const char *authfile, prelude_io_t *fd, const char *addr) 
{
        FILE *file;
        int ret, line;
        char *user, *pass, *client;

        file = fopen(authfile, "r");
        if (! file ) {
                log(LOG_ERR,"couldn't open authentication file %s.\n", authfile);
                return -1;
        }

        ret = auth_read_entry(file, &line, &client, &user, &pass);
        if ( ret < 0 )
                log(LOG_ERR,"couldn't read authentication file %s.\n", authfile);
        else 
                ret = do_auth(fd, user, pass);
        
        fclose(file);
        if (user) free(user);
        if (pass) free(pass);
        if (client) free(client);
        
        return ret;
}





/** 
 * prelude_auth_recv:
 * @authfile: Filename containing username/password pair.
 * @fd: Pointer on a #prelude_io_t object to read authentication from.
 * @addr: Address of the remote host to get login/password associated with.
 *
 * Authenticate the client sending authentication information on @sock.
 *
 * Returns: 0 on success, -1 if an error occured or authentication failed.
 */
int prelude_auth_recv(const char *authfile, prelude_io_t *fd, const char *addr) 
{
        int ret;
        char *user = NULL, *pass = NULL;
        
        ret = get_account_infos(fd, &user, &pass);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't read remote authentication informations.\n");
                return -1;
        }
        
        ret = check_account(authfile, addr, user, pass);
        if ( ret < 0 )
                prelude_io_write_delimited(fd, "failed", 6);
        else
                prelude_io_write_delimited(fd, "ok", 2);
        
        free(user);
        free(pass);
        
        return ret;
}
