/*****
*
* Copyright (C) 2007 PreludeIDS Technologies. All Rights Reserved.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include <assert.h>
#include <gcrypt.h>

#include "glthread/thread.h"

#include "common.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-msg.h"
#include "prelude-failover.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_FAILOVER
#include "prelude-error.h"


#define FAILOVER_CHECKSUM_SIZE       4
#define FAILOVER_JOURNAL_ENTRY_SIZE 20

struct prelude_failover {
        int jfd;
        prelude_io_t *wfd;
        prelude_io_t *rfd;

        uint64_t count;
        uint64_t rindex;

        prelude_bool_t transaction_enabled;
};


typedef union {
        struct {
                uint64_t count;
                uint64_t rindex;
                unsigned char checksum[FAILOVER_CHECKSUM_SIZE];
        } value;
        unsigned char data[FAILOVER_JOURNAL_ENTRY_SIZE];
} failover_journal_entry_t;



static void mask_signal(sigset_t *oldmask)
{
        sigset_t newmask;

        prelude_return_if_fail( sigfillset(&newmask) == 0 );
        prelude_return_if_fail( glthread_sigmask(SIG_BLOCK, &newmask, oldmask) == 0 );
}


static void unmask_signal(sigset_t *oldmask)
{
        prelude_return_if_fail( glthread_sigmask(SIG_SETMASK, oldmask, NULL) == 0 );
}


static void journal_checksum(failover_journal_entry_t *fj, unsigned char *digest, size_t digest_size)
{
        size_t len;

        len = gcry_md_get_algo_dlen(GCRY_MD_CRC32);
        assert(len == digest_size);

        gcry_md_hash_buffer(GCRY_MD_CRC32, digest, fj->data, sizeof(fj->data) - FAILOVER_CHECKSUM_SIZE);
}


static int journal_write(prelude_failover_t *failover)
{
        ssize_t ret;
        size_t rcount = 0;
        failover_journal_entry_t fj;

        fj.value.count = failover->count;
        fj.value.rindex = failover->rindex;
        journal_checksum(&fj, fj.value.checksum, sizeof(fj.value.checksum));

        do {
                ret = write(failover->jfd, fj.data + rcount, sizeof(fj.data) - rcount);
                if ( ret < 0 ) {
                        ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error writing failover journal: %s", strerror(errno));
                        break;
                }

                rcount += ret;
        } while ( ret >= 0 && rcount != sizeof(fj.data) );

        return ret;
}


static int truncate_failover(prelude_failover_t *failover)
{
        off_t off;
        sigset_t oldmask;
        int ret = 0, wfd, rfd;

        wfd = prelude_io_get_fd(failover->wfd);
        rfd = prelude_io_get_fd(failover->rfd);

        mask_signal(&oldmask);

        /*
         * Crash before messages truncate: journal is not updated but on restart
         * rindex equal the size of the file (previous journal update) -> truncate message.
         */

        ret = ftruncate(wfd, 0);
        if ( ret != 0 ) {
                ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error truncating failover: %s", strerror(errno));
                goto error;
        }

        /*
         * Crash after messages truncate, but before journal update: on restart
         * wfd is zero: journal is truncated.
         */
        ret = ftruncate(failover->jfd, 0);
        if ( ret != 0 ) {
                ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error truncating failover journal: %s", strerror(errno));
                goto error;
        }

        off = lseek(rfd, 0, SEEK_SET);
        if ( off == (off_t) -1 ) {
                ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error setting failover read position: %s", strerror(errno));
                goto error;
        }

        off = lseek(failover->jfd, 0, SEEK_SET);
        if ( off == (off_t) -1 ) {
                ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error setting failover journal position: %s", strerror(errno));
                goto error;
        }

        failover->count = failover->rindex = 0;
        journal_write(failover);

error:
        unmask_signal(&oldmask);
        return ret;
}


static int journal_read(prelude_failover_t *failover, failover_journal_entry_t *ent)
{
        ssize_t ret;
        size_t count = 0;

        do {
                ret = read(failover->jfd, ent->data + count, sizeof(ent->data) - count);
                if ( ret < 0 ) {
                        ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error reading failover journal: %s", strerror(errno));
                        break;
                }

                count += ret;

        } while ( count != sizeof(ent->data) );

        return ret;
}



static int journal_check(prelude_failover_t *failover, failover_journal_entry_t *jentry, struct stat *wst)
{
        int ret;
        unsigned char digest[FAILOVER_CHECKSUM_SIZE];

        journal_checksum(jentry, digest, sizeof(digest));

        ret = memcmp(digest, jentry->value.checksum, sizeof(digest));
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_WARN, "Failover: incorrect CRC in journal entry, skipping.\n");
                return -1;
        }

        prelude_log_debug(7, "rindex=%" PRELUDE_PRIu64 " size=%" PRELUDE_PRId64 "\n", jentry->value.rindex, (int64_t) wst->st_size);
        if ( jentry->value.rindex > (uint64_t) wst->st_size ) {
                /*
                 * Latest journal entry has a read index that is higher than the size
                 * of our data file. This mean that the data file is corrupted, and we
                 * cannot recover message after the rindex value. We start over.
                 */
                prelude_log(PRELUDE_LOG_WARN, "Failover: data file corrupted, %" PRELUDE_PRIu64 " messages truncated.\n",
                            jentry->value.count);

                jentry->value.rindex = jentry->value.count = 0;
                truncate_failover(failover);
                return 0;
        }

        else if ( jentry->value.rindex == (uint64_t) wst->st_size ) {
                /*
                 * Read-Index and size are the same, but file was not truncated.
                 */
                jentry->value.rindex = jentry->value.count = 0;
                truncate_failover(failover);
        }

        return 0;
}


static int journal_read_last_entry(prelude_failover_t *failover,
                                   failover_journal_entry_t *jentry, struct stat *jst, struct stat *wst)
{
        int ret, garbage;
        off_t lret, offset = jst->st_size;

        garbage = offset % FAILOVER_JOURNAL_ENTRY_SIZE;
        if ( garbage != 0 ) {
                /*
                 * The journal is corrupted, not on the correct boundary.
                 */
                prelude_log(PRELUDE_LOG_WARN, "Failover: journal corrupted, recovering from invalid boundary.\n");
                offset -= garbage;
        }

        do {
                offset -= FAILOVER_JOURNAL_ENTRY_SIZE;

                lret = lseek(failover->jfd, offset, SEEK_SET);
                if ( lret == (off_t) -1 )
                        return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error seeking to end of journal: %s", strerror(errno));

                ret = journal_read(failover, jentry);
                if ( ret < 0 )
                        return ret;

                prelude_log_debug(7, "[%" PRELUDE_PRId64 "] found jentry with count=%" PRELUDE_PRIu64 " rindex=%" PRELUDE_PRIu64 "\n", (int64_t) offset, jentry->value.count, jentry->value.rindex);

                ret = journal_check(failover, jentry, wst);

        } while ( offset > 0 && ret != 0 );

        if ( ret != 0 ) {
                truncate_failover(failover);
                return 0;
        }

        return ret;
}



static int journal_initialize(prelude_failover_t *failover, const char *filename)
{
        int ret;
        off_t off;
        struct stat jst, wst;
        failover_journal_entry_t jentry;

        failover->jfd = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
        if ( failover->jfd < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not open '%s': %s", filename, strerror(errno));

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        fcntl(failover->jfd, F_SETFD, fcntl(failover->jfd, F_GETFD) | FD_CLOEXEC);
#endif

        ret = fstat(failover->jfd, &jst);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not stat failover journal: %s", strerror(errno));

        ret = fstat(prelude_io_get_fd(failover->wfd), &wst);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not stat failover journal: %s", strerror(errno));

        if ( jst.st_size == 0 && wst.st_size > 0 ) {
                truncate_failover(failover);
                prelude_log(PRELUDE_LOG_WARN, "Failover inconsistency: message data with no journal.\n");
                return 0;
        }

        else if ( jst.st_size == 0 && wst.st_size == 0 )
                return 0;

        memset(&jentry.value, 0, sizeof(jentry.value));

        ret = journal_read_last_entry(failover, &jentry, &jst, &wst);
        if ( ret < 0 )
                return ret;

        failover->count = jentry.value.count;
        failover->rindex = jentry.value.rindex;

        off = lseek(prelude_io_get_fd(failover->rfd), failover->rindex, SEEK_SET);
        if ( off == (off_t) -1 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error setting failover read offset: %s", strerror(errno));

        return 0;
}


#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
static int lock_cmd(const char *filename, int fd, int cmd, int type)
{
        int ret;
        struct flock lock;

        lock.l_type = type;       /* write lock */
        lock.l_start = 0;         /* from offset 0 */
        lock.l_whence = SEEK_SET; /* at the beginning of the file */
        lock.l_len = 0;           /* until EOF */

        ret = fcntl(fd, cmd, &lock);
        if ( ret < 0 ) {
                if ( errno == EAGAIN || errno == EACCES )
                        return 0;

                return prelude_error_verbose(prelude_error_code_from_errno(errno), "error locking '%s': %s",
                                             filename, strerror(errno));
        }

        return 1;
}
#endif



static int open_exclusive(const char *filename, int flags, int *fd)
{
        int ret = 1;

        *fd = open(filename, flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
        if ( *fd < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error opening '%s': %s", filename, strerror(errno));

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        ret = lock_cmd(filename, *fd, F_SETLK, F_RDLCK|F_WRLCK);
        if ( ret <= 0 )
                close(*fd);
#endif

        return ret;
}


static int get_failover_data_filename_and_fd(const char *dirname, char *filename, size_t size)
{
        DIR *dir;
        int i = 0, fd, ret = 0;
        struct dirent *de;

        dir = opendir(dirname);
        if ( ! dir )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error opening '%s': %s", dirname, strerror(errno));

        while ( (de = readdir(dir)) && ret != 1 ) {

                if ( strlen(de->d_name) <= 4 || ! isdigit(de->d_name[4]) )
                        continue;

                if ( strncmp(de->d_name, "data", 4) != 0 || strchr(de->d_name, '.') )
                        continue;

                ret = snprintf(filename, size, "%s/%s", dirname, de->d_name);
                if ( ret < 0 || (size_t) ret >= size )
                        continue;

                ret = open_exclusive(filename, O_CREAT|O_WRONLY|O_APPEND, &fd);
                if ( ret < 0 )
                        return ret;
        }

        while ( ret != 1 ) {
                ret = snprintf(filename, size, "%s/data%d", dirname, i++);
                if ( ret < 0 || (size_t) ret >= size )
                        continue;

                ret = open_exclusive(filename, O_CREAT|O_WRONLY|O_APPEND, &fd);
                if ( ret < 0 )
                        return ret;

        }

        closedir(dir);

        return fd;
}


int prelude_failover_commit(prelude_failover_t *failover, prelude_msg_t *msg)
{
        /*
         * Make sure that we don't go down zero. This might happen with a message
         * data file with more message than what the journal specified.
         */
        if ( failover->count > 0 )
                failover->count--;

        failover->rindex += prelude_msg_get_len(msg);
        journal_write(failover);

        return 0;
}


int prelude_failover_rollback(prelude_failover_t *failover, prelude_msg_t *msg)
{
        off_t off;

        off = lseek(prelude_io_get_fd(failover->rfd), - prelude_msg_get_len(msg), SEEK_CUR);
        if ( off == (off_t) -1 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error setting failover read position: %s", strerror(errno));

        return 0;
}


ssize_t prelude_failover_get_saved_msg(prelude_failover_t *failover, prelude_msg_t **msg)
{
        int ret;

        *msg = NULL;

        ret = prelude_msg_read(msg, failover->rfd);
        if ( ret < 0 ) {
                uint64_t bkp = failover->count;

                truncate_failover(failover);

                if ( prelude_error_get_code(ret) == PRELUDE_ERROR_EOF )
                        return 0;
                else
                        return prelude_error_verbose(PRELUDE_ERROR_GENERIC,
                                                     "%" PRELUDE_PRIu64 " messages failed to recover: %s",
                                                     bkp, prelude_strerror(ret));
        }

        if ( ! failover->transaction_enabled )
                prelude_failover_commit(failover, *msg);

        return prelude_msg_get_len(*msg);
}



int prelude_failover_save_msg(prelude_failover_t *failover, prelude_msg_t *msg)
{
        int ret;
        sigset_t oldset;

        mask_signal(&oldset);

        do {
                ret = prelude_msg_write(msg, failover->wfd);
        } while ( ret < 0 && errno == EINTR );

        if ( ret < 0 )
                goto error;

        if ( ! prelude_msg_is_fragment(msg) ) {
                failover->count++;
                journal_write(failover);
        }

error:
        unmask_signal(&oldset);

        return ret;
}



int prelude_failover_new(prelude_failover_t **out, const char *dirname)
{
        mode_t mode;
        size_t flen;
        int ret, wfd, rfd;
        char filename[PATH_MAX];
        prelude_failover_t *new;

        mode = umask(S_IRWXO);

        ret = mkdir(dirname, S_IRWXU|S_IRWXG);
        if ( ret < 0 && errno != EEXIST ) {
                umask(mode);
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not create directory '%s': %s", dirname, strerror(errno));
        }

        wfd = get_failover_data_filename_and_fd(dirname, filename, sizeof(filename));
        if ( wfd < 0 ) {
                umask(mode);
                return wfd;
        }

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        fcntl(wfd, F_SETFD, fcntl(wfd, F_GETFD) | FD_CLOEXEC);
#endif

        rfd = open(filename, O_RDONLY);
        if ( rfd < 0 ) {
                umask(mode);
                close(wfd);
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not open '%s' for reading: %s", filename, strerror(errno));
        }

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        fcntl(rfd, F_SETFD, fcntl(rfd, F_GETFD) | FD_CLOEXEC);
#endif

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                umask(mode);
                close(rfd);
                close(wfd);
                return prelude_error_from_errno(errno);
        }

        new->jfd = -1;

        ret = prelude_io_new(&new->wfd);
        if ( ret < 0 ) {
                umask(mode);
                close(rfd);
                close(wfd);
                free(new);
                return ret;
        }

        ret = prelude_io_new(&new->rfd);
        if ( ret < 0 ) {
                umask(mode);
                close(rfd);
                close(wfd);
                free(new);
                return ret;
        }

        prelude_io_set_sys_io(new->wfd, wfd);
        prelude_io_set_sys_io(new->rfd, rfd);

        flen = strlen(filename);

        ret = snprintf(filename + flen, sizeof(filename) - flen, ".journal");
        if ( ret < 0 || (size_t) ret >= (sizeof(filename) - flen) ) {
                umask(mode);
                prelude_failover_destroy(new);
                return -1;
        }

        ret = journal_initialize(new, filename);
        if ( ret < 0 ) {
                umask(mode);
                prelude_failover_destroy(new);
                return ret;
        }

        umask(mode);
        *out = new;

        return 0;
}



void prelude_failover_destroy(prelude_failover_t *failover)
{
        close(failover->jfd);

        if ( failover->wfd ) {
                prelude_io_close(failover->wfd);
                prelude_io_destroy(failover->wfd);
        }

        if ( failover->rfd ) {
                prelude_io_close(failover->rfd);
                prelude_io_destroy(failover->rfd);
        }

        free(failover);
}



void prelude_failover_set_quota(prelude_failover_t *failover, size_t limit)
{
        /* FIXME: quota */
}



unsigned long prelude_failover_get_deleted_msg_count(prelude_failover_t *failover)
{
        /* FIXME: quota */
        return 0;
}



unsigned long prelude_failover_get_available_msg_count(prelude_failover_t *failover)
{
        return (unsigned long) failover->count;
}


void prelude_failover_enable_transaction(prelude_failover_t *failover)
{
        failover->transaction_enabled = TRUE;
}


void prelude_failover_disable_transaction(prelude_failover_t *failover)
{
        failover->transaction_enabled = FALSE;
}
