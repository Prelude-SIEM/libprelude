/*****
*
* Copyright (C) 2000, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <sys/time.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>

#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-auth.h"



extern char *crypt(const char *key, const char *salt);




static char *get_random_salt(char *buf, size_t size) 
{
        int num, i;
        struct timeval tv;
        char sc[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

        gettimeofday(&tv, NULL);
        srand(tv.tv_sec ^ tv.tv_usec ^ getpid());

        for ( i = 0; i < size; i++ ) {
                
                num = rand() % (sizeof(sc) - 1);
                buf[i] = sc[num];
        }
        
        return buf;
}




static int get_password_salt(const char *pass, char buf[3]) 
{
        if ( strlen(pass) < sizeof(buf) ) {
                log(LOG_ERR, "couldn't gather salt from empty password.\n");
                return -1;
        }
        
        buf[0] = pass[0];
        buf[1] = pass[1];
        buf[2] = '\0';

        return 0;
}




/*
 * Parse the line 'line' comming from the authentication file,
 * and store the username and password in 'user' and 'pass'
 * passed arguments.
 */
static int parse_auth_line(char *line, char **user, char **pass) 
{
        char *end;

        end = strchr(line, ':');
        if ( ! end ) {
                log(LOG_INFO, "couldn't found username delimiter.\n");
                return -1;
        }

        *end = '\0';
        *user = strdup(line);
        if ( ! *user ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }
        
        line = end + 1;
        
        end = strchr(line , ':');
        if (! end ) {
                log(LOG_INFO, "couldn't found password delimiter.\n");
                free(*user);
                return -1;
        }

        *end = '\0';
        *pass = strdup(line);
        if ( ! *pass ) {
                log(LOG_ERR, "memory exhausted.\n");
                free(*user);
                return -1;
        }
        
        return 0;
}



/*
 * Read the next entry in the authentication file pointed by fd.
 */
static int auth_read_entry(FILE *fd, int *line, char **user, char **pass) 
{
        int ret;
        char buf[1024];

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





static int cmp_cleartext_with_crypted(const char *cleartext_pass, const char *crypted_pass) 
{
        int ret;
        char salt[3], *cpass;
        
        ret = get_password_salt(crypted_pass, salt);
        if ( ret < 0 )       
                return -1;
        
        cpass = crypt(cleartext_pass, salt);
        if ( ! cpass ) 
                return -1;

        ret = strcmp(cpass, crypted_pass);
        if ( ret != 0 )
                return -1;

        return 0;
}




/*
 *
 */
static int cmp(const char *given_user, const char *user,
               const char *given_pass, const char *pass) 
{
        int ret;
        
        ret = strcmp(given_user, user);
        if ( ret != 0 )
                return -1;

        ret = cmp_cleartext_with_crypted(given_pass, pass);
        if ( ret < 0 ) 
                return -1;

        return 0;
}



/*
 *
 */
static int check_account(const char *authfile,
                         const char *given_user, const char *given_pass) 
{
        FILE *fd;
        int line = 0, ret;
        char *user, *pass;
        
        fd = fopen(authfile, "r");
        if ( ! fd ) {
                if ( errno == ENOENT )
                        /*
                         * not existing authentication file is not an error.
                         */
                        return -1;
                
                log(LOG_ERR, "couldn't open %s for reading.\n", authfile);
                return -1;
        }

        while ( auth_read_entry(fd, &line, &user, &pass) == 0 ) {

                ret = cmp(given_user, user, given_pass, pass);
                
                free(user);
                free(pass);
                
                if ( ret == 0 ) {
                        fclose(fd);
                        return 0;
                }
        }
        
        fclose(fd);

        return -1;
}




/*
 * Open authentication filename in append mode,
 * with stream positionned at the end of the file,
 *
 * If the authentication file does not exist, it is
 * created with permission 600.
 */
static FILE *open_auth_file(const char *filename, uid_t uid) 
{
        int ret;
        FILE *fd;
        
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

        ret = fchown(fileno(fd), uid, -1);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't change %s to UID %d.\n", filename, uid);
                fclose(fd);
                return NULL;
        }
        
        return fd;
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
                log(LOG_ERR, "couldn't seek to end of file.\n");
                return -1;
        }

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
        
        if ( ! fgets(buf, sizeof(buf), stdin) )
                return NULL;
        
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




/*
 * Check if user 'user' already exist.
 */
static int account_already_exist(FILE *fd, const char *nuser) 
{
        int line = 0;
        char *user, *pass;
        
        rewind(fd);
        while (auth_read_entry(fd, &line, &user, &pass) == 0 ) {

                if ( strcmp(nuser, user) == 0 ) {
                        fprintf(stderr, "username %s already exist.\n", nuser);
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
        char buf[5];

        do {
                fprintf(stderr, "Register user \"%s\" ? [y/n] : ", user);

                if ( ! fgets(buf, sizeof(buf), stdin) )
                        continue;
                
        } while ( buf[0] != 'y' && buf[0] != 'n' );

        return (buf[0] == 'y') ? 0 : -1;
}




/*
 * Call subroutine in order to create an account,
 * and do all the necessary test to see if it is valid.
 */
static int ask_account_infos(FILE *fd, char **user, char **pass) 
{
        int ret;
        
        *user = ask_username();
        if ( ! *user ) {
                fclose(fd);
                return -1;
        }

        ret = account_already_exist(fd, *user);
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





/**
 * prelude_auth_create_account:
 * @filename: The filename to store account in.
 * @user: Address of a pointer where the created username will be stored.
 * @pass: Address of a pointer where the created password will be stored.
 * @crypted: Specify wether the password should be crypted using crypt().
 * @uid: UID of authentication file owner.
 *
 * Ask for a new account creation which will be stored into 'filename'
 * which is the authentication file. Uppon success, @user and @pass will
 * be updated to point on the configured username and password.
 *
 * Returns: 0 on sucess, -1 otherwise
 */
int prelude_auth_create_account(const char *filename, char **user, char **pass, int crypted, uid_t uid) 
{
        int ret;
        FILE *fd;
        char *cpass, salt[3];

        fd = open_auth_file(filename, uid);
        if ( ! fd ) 
                return -1;

        ret = ask_account_infos(fd, user, pass);
        if ( ret < 0 ) {
                fclose(fd);
                return -1;
        }

        if ( crypted )
                cpass = crypt(*pass, get_random_salt(salt, sizeof(salt)));
        else
                cpass = *pass;
        
        ret = comfirm_account_creation(*user);
        if ( ret == 0 ) 
                write_account(fd, *user, cpass);

        fclose(fd);

        return ret;
}




/**
 * prelude_auth_create_account_noprompt:
 * @filename: The filename to store account in.
 * @user: Username to create.
 * @pass: Password associated with username.
 * @crypted: Specify wether the password should be crypted using crypt().
 * @uid: UID of authentication file owner.
 *
 * Create specified account.
 *
 * Returns: 0 on sucess, -1 otherwise
 */
int prelude_auth_create_account_noprompt(const char *filename, const char *user,
                                         const char *pass, int crypted, uid_t uid) 
{
        FILE *fd;
        char salt[2];
        const char *cpass;
        
        fd = open_auth_file(filename, uid);
        if ( ! fd ) 
                return -1;

        if ( crypted )
                cpass = crypt(pass, get_random_salt(salt, sizeof(salt)));
        else
                cpass = pass;

        write_account(fd, user, cpass);
        fclose(fd);

        return 0;
}




/**
 * prelude_auth_read_entry:
 * @authfile: Filename containing username/password pair.
 * @wanted_user: Pointer to an username.
 * @wanted_pass: Pointer to a password.
 * @user: Address of a pointer where username should be stored.
 * @pass: Address of a pointer where password should be stored.
 *
 * prelude_auth_read_entry() try to find @wanted_user in @authfile.
 *
 * If @wanted_user is NULL, the first entry in @authfile is returned.
 *
 * If @wanted_password is not NULL and that @wanted_user is found,
 * the password will be compared and an error will be returned if
 * they don't match.
 *
 * Returns: 0 on success, -1 for generic error, @password_does_not match,
 * @user_does_not_exist.
 */
int prelude_auth_read_entry(const char *authfile, const char *wanted_user,
                            const char *wanted_pass, char **user, char **pass) 
{
        FILE *file;
        int line = 0, ret;
        
        file = fopen(authfile, "r");
        if ( ! file ) {
                if ( errno == ENOENT )
                        return user_does_not_exist;

                return -1;
        }
        
        while ( auth_read_entry(file, &line, user, pass) == 0 ) {

                if ( ! wanted_user )
                        /*
                         * return first entry.
                         */
                        return 0;

                if ( strcmp(wanted_user, *user) != 0 ) {
                        free(*user);
                        free(*pass);
                        continue;
                }

                if ( wanted_pass ) {
                        ret = cmp_cleartext_with_crypted(wanted_pass, *pass);
                        if ( ret < 0 )
                                return password_does_not_match;
                }
                
                return 0;
        }
        
        return user_does_not_exist;
}




/** 
 * prelude_auth_check:
 * @authfile: Filename containing username/password pair.
 * @user: Pointer on an username.
 * @pass: Pointer on a password.
 *
 * Check the @used / @pass pair match an entry in the @authfile file.
 *
 * Returns: 0 on success, -1 if an error occured or authentication failed.
 */
int prelude_auth_check(const char *authfile, const char *user, const char *pass) 
{
        return check_account(authfile, user, pass);
}






