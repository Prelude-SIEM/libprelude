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

#include "auth-common.h"
#include "common.h"


extern char *crypt(const char *key, const char *salt);



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



/*
 * Ask for an username, then return it.
 */
static char *ask_username(void) 
{
        char buf[1024];
        
        printf("\nUsername Prelude should use to authenticate : ");
        
        fgets(buf, sizeof(buf), stdin);

        /* strip \n */
        buf[strlen(buf) - 1] = '\0';
        
        return strdup(buf);
}



/*
 * Check if user 'user' already exist.
 */
static int user_already_exist(FILE *fd, const char *nuser) 
{
        int line = 0;
        char *user, *pass;
        
        rewind(fd);
        while ( auth_read_entry(fd, &line, &user, &pass) == 0 ) {
                if ( strcmp(nuser, user) == 0 ) {
                        fprintf(stderr, "user %s already exist.\n", nuser); 
                        return -1;
                }
        }

        return 0;
}



/*
 * Call subroutine in order to create an account,
 * and do all the necessary test to see if it is valid.
 */
static int ask_account_infos(FILE *fd, char **user, char **pass) 
{
        int ret;

        fprintf(stderr, "You need to register an user for unencrypted connection.\n");
        
        *user = ask_username();
        if ( ! *user ) {
                fclose(fd);
                return -1;
        }

        ret = user_already_exist(fd, *user);
        if ( ret < 0 ) {
                fclose(fd);
                free(*user);
                return -1;
        }
        
        *pass = ask_password();
        if ( ! *pass ) {
                free(*user);
                fclose(fd);
                return -1;
        }
        
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
 * Parse the line 'line' comming from the authentication file,
 * and store the username and password in 'user' and 'pass'
 * passed arguments.
 */
static int parse_auth_line(char *line, char **user, char **pass) 
{
        char *tmp;
        
        tmp = strtok(line, ":");
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
 * Write new account (consisting of username 'user'
 * and password 'pass' to the authentication file.
 */
static int write_account(FILE *fd, const char *user, const char *pass) 
{
        int ret;
        
        ret = fseek(fd, 0, SEEK_END);
        if ( ret < 0 ) {
                log(LOG_ERR, "seek.\n");
                return -1;
        }

        fwrite(user, 1, strlen(user), fd);
        fwrite(":", 1, 1, fd);
        fwrite(pass, 1, strlen(pass), fd);
        fwrite(":\n", 1, 2, fd);

        return 0;
}




/*
 * Verify that the authentication file 'filename',
 * exist and is valid (here, valid mean that it contain
 * at least one entry, we use auth_read_entry to see that).
 */
static int exist_and_valid(const char *filename) 
{
        FILE *fd;
        int ret, line = 0;
        char *user, *pass;
        
        ret = access(filename, F_OK);
        if ( ret < 0 )
                return -1;

        fd = fopen(filename, "r");
        if ( ! fd ) {
                log(LOG_ERR, "couldn't open %s for reading.\n", filename);
                return -1;
        }
        
        ret = auth_read_entry(fd, &line, &user, &pass);
        if ( ret < 0 ) {
                fclose(fd);
                return -1;
        }

        free(user);
        free(pass);
        fclose(fd);
        
        return 0;
}




/**
 * auth_create_account:
 * @filename: The filename to store account in.
 * @crypted: Tell the pass have to be crypted with the crypt function.
 *
 * Ask for a new account creation which will be stored into 'filename'
 * which is the authentication file. crypted mean if the password will
 * be written in a crypted fashion or not.
 *
 * Returns: 0 on sucess, -1 otherwise
 */
int auth_create_account(const char *filename, const int crypted) 
{
        int ret;
        FILE *fd;
        char *user, *pass;

        fd = open_auth_file(filename);
        if ( ! fd ) 
                return -1;

        ret = ask_account_infos(fd, &user, &pass);
        if ( ret < 0 ) {
                fclose(fd);
                return -1;
        }
        
        ret = comfirm_account_creation(user);
        if ( ret == 0 ) {
#ifdef HAVE_CRYPT
                if ( crypted ) {
                        char *cpass;
                        
                        cpass = strdup(crypt(pass, SALT));
                        free(pass);
                        pass = cpass;
                }
#endif
                write_account(fd, user, pass);
        }

        free(user);
        free(pass);
        
        fclose(fd);

        return ret;
}




/**
 * auth_read_entry:
 * @fd: File descriptor pointing to the authentication file.
 * @line: where the current read line will be stored.
 * @user: where the username read will be stored.
 * @pass: where the password read will be stored.
 *
 * Read the next entry in the authentication file pointed by fd,
 * store the current line, user, and pass in their corresponding pointer.
 * If not NULL, user & pass pointer must be freed.
 *
 * Return: 0 on success, -1 otherwise.
 */
int auth_read_entry(FILE *fd, int *line, char **user, char **pass) 
{
        char buf[1024];
        int ret;

        if ( fgets(buf, sizeof(buf), fd) ) {
                
                *line += 1;
                ret = parse_auth_line(buf, user, pass);
                if ( ret < 0 ) {
                        log(LOG_ERR, "couldn't parse line %d.\n", *line);
                        return -1;
                }
                
                return 0;
        }
        
        return -1;
}




/**
 * auth_file_exist_or_create:
 * @filename: The authentication filename .
 * @crypted: wether to crypt if password if we create an account.
 *
 * If the authentication file 'filename' does not exist, create it,
 * asking for the creation of a new user. crypted mean if the password
 * will be written in a crypted fashion or not.
 *
 * Return: 0 on success, -1 otherwise.
 */
int auth_file_exist_or_create(const char *filename, int crypted) 
{
        int ret;

        ret = exist_and_valid(filename);
        if ( ret < 0 )
                return auth_create_account(filename, crypted);

        return 0;
}














