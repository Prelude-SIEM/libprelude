/*****
*
* Copyright (C) 2000, 2002, 2003, 2004 PreludeIDS Technologies. All Rights Reserved.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>

#include "prelude-inttypes.h"
#include "common.h"
#include "config-engine.h"
#include "variable.h"
#include "prelude-log.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CONFIG_ENGINE
#include "prelude-error.h"


struct config {
        char *filename; /* filename for this session */
        char **content; /* content of the file */
        prelude_bool_t need_sync;  /* does the file need to be synced on disk ? */
        unsigned int elements;   /* array number of elements */
};




static void free_val(char **val)
{
        if ( ! *val )
                return;

        free(*val);
        *val = NULL;
}



static prelude_bool_t is_line_commented(const char *line)
{
        line += strspn(line, " \t\r");
        return ( *line == '#' ) ? TRUE : FALSE;
}




/*
 * If line contains a section, returns a pointer to
 * the beginning of the section name.
 */
static prelude_bool_t is_section(const char *line)
{
        line += strspn(line, " \t\r\n");

        if ( *line == '[' && strchr(line, ']') )
                return TRUE;

        return FALSE;
}




/*
 * Frees old line pointed by line,
 * and sets to the new one.
 */
static void op_modify_line(char **line, char *nline)
{
        if ( ! nline )
                return;

        free(*line); *line = nline;
}



static int op_delete_line(config_t *cfg, unsigned int start, unsigned int end)
{
        unsigned int i, j;

        if ( ! cfg->elements )
                return 0;

        if ( start >= end || end > cfg->elements )
                return -1;

        for ( i = start; i < end; i++ ) {
                free(cfg->content[i]);
                cfg->content[i] = NULL;
        }

        for ( i = end, j = start; i < cfg->elements; i++ )
                cfg->content[j++] = cfg->content[i];

        cfg->elements -= end - start;

        cfg->content = _prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return prelude_error_from_errno(errno);

        return 0;
}



/*
 * Appends a line to an array of line.
 * Takes the address of the array and the line to append as arguments.
 *
 * list must be NULL the first time this function is called,
 * in order to initialize indexing variable.
 */
static int op_append_line(config_t *cfg, char *line)
{
        if ( ! line )
                return 0;

        if ( cfg->elements + 1 < cfg->elements )
                return -1;

        cfg->elements++;

        cfg->content = _prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return prelude_error_from_errno(errno);

        cfg->content[cfg->elements - 1] = line;

        return 0;
}



static unsigned int adjust_insertion_point(config_t *cfg, unsigned int l)
{
        /*
         * We want to insert this entry at the end of the current section.
         */
        while ( l < cfg->elements ) {
                l++;

                if ( l == cfg->elements )
                        break;

                if ( is_section(cfg->content[l]) )
                        break;
        }

        return l;
}



/*
 *
 */
static int op_insert_line(config_t *cfg, char *line, unsigned int lins)
{
        unsigned int i;

        if ( lins > cfg->elements || ! line )
                return -1;

        if ( cfg->elements + 1 < cfg->elements )
                return -1;

        cfg->content = _prelude_realloc(cfg->content, ++cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return prelude_error_from_errno(errno);

        for ( i = cfg->elements - 2; i >= lins; i-- ) {
                cfg->content[i + 1] = cfg->content[i];
                if ( i == 0 )
                        break;
        }

        cfg->content[lins] = line;

        return 0;
}




/*
 * Loads filename into memory, we use append_line() for that.
 */
static int load_file_in_memory(config_t *cfg)
{
        int ret;
        FILE *fd;
        size_t len;
        prelude_string_t *out;
        char line[1024], *ptr, *tmp;

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        fd = fopen(cfg->filename, "r");
        if ( ! fd ) {
                prelude_string_destroy(out);
                return prelude_error_verbose(prelude_error_code_from_errno(errno), "could not open '%s' for reading: %s",
                                             cfg->filename, strerror(errno));
        }

        do {
                len = 0;
                ptr = fgets(line, sizeof(line), fd);
                if ( ptr ) {
                        len = strlen(line);

                        if ( line[len - 1] == '\n' )
                                line[len - 1] = 0;

                        ret = prelude_string_cat(out, line);
                        if ( ret < 0 )
                                goto err;

                        if ( line[len - 1] != 0 )
                                continue;
                }

                ret = prelude_string_get_string_released(out, &tmp);
                if ( ret < 0 )
                        goto err;

                if ( ! tmp )
                        tmp = strdup("");

                ret = op_append_line(cfg, tmp);
                if ( ret < 0 ) {
                        free(tmp);
                        goto err;
                }

                prelude_string_clear(out);
        } while ( ptr );

 err:
        prelude_string_destroy(out);
        fclose(fd);

        return 0;
}



static int strip_value(char **out, const char *in, size_t tlen)
{
        size_t slen, elen;
        prelude_bool_t have_start_quote = FALSE;

        *out = NULL;

        in += slen = strspn(in, " \t\r");
        if ( *in == '"' ) {
                in++; slen++;
                have_start_quote = TRUE;
        }

        elen = tlen - slen;
        if ( ! elen )
                return 0;

        while ( in[elen - 1] == ' ' || in[elen - 1] == '\t' || in[elen - 1] == '\r' )
                elen--;

        if ( have_start_quote && elen ) {
                if ( in[elen - 1] == '"' )
                        elen--;
                else {
                        in--;
                        elen++;
                }
        }

        if ( ! elen )
                return 0;

        *out = strndup(in, elen);

        return (*out) ? 0 : prelude_error_from_errno(errno);
}



static int value_resolve_variable(const char *ptr, char **out_p)
{
        int ret;
        size_t i;
        char buf[512];
        const char *tmp;
        prelude_string_t *out;

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        while ( *ptr ) {
                if ( *ptr == '$' ) {
                        tmp = ptr;
                        i = 0;

                        do {
                                buf[i++] = *ptr++;
                        } while ( *ptr && *ptr != ' ' && *ptr != '$' && i < sizeof(buf) - 1);

                        buf[i] = 0;

                        if ( ! variable_get(buf + 1) )
                                ptr = tmp;
                        else {
                                prelude_string_cat(out, variable_get(buf + 1));
                                continue;
                        }
                }

                prelude_string_ncat(out, ptr, 1);
                ptr++;
        }

        ret = prelude_string_get_string_released(out, out_p);
        prelude_string_destroy(out);

        return ret;
}




static int parse_buffer(const char *str, char **entry, char **value)
{
        int ret;
        size_t len;
        char *val;
        const char *ptr;

        *value = *entry = NULL;
        if ( ! *str )
                return -1;

        ptr = strchr(str, '=');
        len = (ptr) ? (size_t) (ptr - str) : strlen(str);

        ret = strip_value(entry, str, len);
        if ( ret < 0 )
                return ret;

        if ( ! ptr )
                return 0;

        ret = strip_value(&val, ptr + 1, strlen(ptr + 1));
        if ( ret < 0 )
                return ret;

        if ( val )
                ret = value_resolve_variable(val, value);

        free_val(&val);
        if ( ret < 0 )
                return ret;

        if ( **entry == '$' )
                ret = variable_set((*entry) + 1, *value);

        return ret;
}


static int parse_section_buffer(const char *buf, char **entry, char **value, prelude_bool_t default_value)
{
        int ret;
        char *ptr;

        buf += strspn(buf, "\n [");

        ptr = strchr(buf, ']');
        if ( ptr )
                *ptr = 0;

        ret = parse_buffer(buf, entry, value);
        if ( ptr )
                *ptr = ']';

        if ( ! *value && default_value )
                *value = strdup("default");

        return ret;
}




static int search_section(config_t *cfg, const char *section, unsigned int i)
{
        int ret;
        char *entry, *value, *wentry, *wvalue;

        if ( ! cfg->content )
                return -1;

        ret = parse_section_buffer(section, &wentry, &wvalue, TRUE);
        if ( ret < 0 )
                return ret;

        for ( ; i < cfg->elements; i++ ) {

                if ( is_line_commented(cfg->content[i]) )
                        continue;

                if ( ! is_section(cfg->content[i]) )
                        continue;

                ret = parse_section_buffer(cfg->content[i], &entry, &value, TRUE);
                if ( ret < 0 )
                        continue;

                ret = strcasecmp(entry, wentry);
                free(entry);

                if ( ret != 0 ) {
                        free(value);
                        continue;
                }

                ret = strcasecmp(value, wvalue);
                free(value);

                if ( ret == 0 ) {
                        free(wentry);
                        free(wvalue);
                        return i;
                }
        }

        free(wentry);
        free(wvalue);

        return -1;
}





/*
 * Search an entry (delimited by '=' character) in content.
 * returns the line number matching 'entry' or -1.
 */
static int search_entry(config_t *cfg, const char *section,
                        const char *entry, unsigned int *index, char **eout, char **vout)
{
        int ret;
        unsigned int i = *index;

        if ( ! cfg->content || i >= cfg->elements )
                return -1;

        if ( section && ! index ) {

                ret = search_section(cfg, section, 0);
                if ( ret < 0 )
                        return ret;

                i = (unsigned int) ret + 1;
        }

        for ( ; i < cfg->elements; i++ ) {

                if ( section && is_section(cfg->content[i]) )
                        return -1;

                ret = parse_buffer(cfg->content[i], eout, vout);
                if ( ret < 0 || ! *eout )
                        continue;

                ret = strcmp(entry, *eout);
                if ( ret == 0 ) {
                        *index = i;
                        return 0;
                }

                free_val(eout);
                free_val(vout);
        }

        return -1;
}



/*
 * Creates a new line using entry 'entry' & value 'val'.
 */
static char *create_new_line(const char *entry, const char *val)
{
        int ret;
        char *line;
        size_t len = 0;

        if ( ! entry )
                return NULL;

        if ( val )
                len = strlen(entry) + strlen(val) + 2;
        else
                len = strlen(entry) + 1;

        line = malloc(len);
        if (! line )
                return NULL;

        if ( val )
                ret = snprintf(line, len, "%s=%s", entry, val);
        else
                ret = snprintf(line, len, "%s", entry);

        if ( ret < 0 || (size_t) ret >= len ) {
                free(line);
                return NULL;
        }

        return line;
}



/*
 * Only called if the memory dump of the file was modified,
 * will write the new content to filename 'filename'.
 */
static int sync_and_free_file_content(config_t *cfg)
{
        FILE *fd;
        unsigned int i;
        size_t ret, len;

        fd = fopen(cfg->filename, "w");
        if ( ! fd )
                return prelude_error_verbose(prelude_error_code_from_errno(errno), "could not open '%s' for writing: %s",
                                             cfg->filename, strerror(errno));

        for ( i = 0; i < cfg->elements; i++ ) {
                len = strlen(cfg->content[i]);

                ret = fwrite(cfg->content[i], 1, len, fd);
                if ( ret != len && ferror(fd) )
                        prelude_log(PRELUDE_LOG_ERR, "error writing content to '%s': %s", cfg->filename, strerror(errno));

                if ( i + 1 != cfg->elements ) {
                        ret = fwrite("\n", 1, 1, fd);
                        if ( ret != 1 && ferror(fd) )
                                prelude_log(PRELUDE_LOG_ERR, "error writing content to '%s': %s", cfg->filename, strerror(errno));
                }

                free(cfg->content[i]);
        }

        fclose(fd);
        free(cfg->content);

        return 0;
}



/*
 * frees the 'content' array, and its pointer.
 * 'content' is the content of the file loaded into memory.
 */
static void free_file_content(config_t *cfg)
{
        unsigned int i;

        for ( i = 0; i < cfg->elements; i++ )
                free(cfg->content[i]);

        free(cfg->content);
}



/*
 *
 */
static int new_entry_line(config_t *cfg, const char *entry, const char *val, unsigned int *index)
{
        int ret;
        char *eout, *vout;

        ret = search_entry(cfg, NULL, entry, index, &eout, &vout);
        if ( ret < 0 )
                return op_insert_line(cfg, create_new_line(entry, val), *index);

        free_val(&eout);
        free_val(&vout);

        op_modify_line(&cfg->content[*index], create_new_line(entry, val));

        return 0;
}



/*
 *
 */
static int new_section_line(config_t *cfg, const char *section,
                            const char *entry, const char *val, unsigned int *index)
{
        int ret;
        char *eout, *vout;
        unsigned int eindex;

        ret = search_section(cfg, section, *index);
        if ( ret < 0 ) {
                char buf[1024];

                snprintf(buf, sizeof(buf), "[%s]", section);

                if ( *index )
                        *index = adjust_insertion_point(cfg, *index);
                else
                        *index = cfg->elements;

                ret = op_insert_line(cfg, strdup(buf), *index);
                if ( ret < 0 )
                        return ret;

                return (! entry) ? 0 : op_insert_line(cfg, create_new_line(entry, val), *index + 1);
        }

        *index = ret;
        if ( ! entry )
                return 0;

        eindex = *index + 1;

        ret = search_entry(cfg, section, entry, &eindex, &eout, &vout);
        if ( ret < 0 )
                return op_insert_line(cfg, create_new_line(entry, val), eindex);

        free_val(&eout);
        free_val(&vout);

        op_modify_line(&cfg->content[eindex], create_new_line(entry, val));

        return 0;
}




/**
 * _config_set:
 * @cfg: Configuration file identifier.
 * @section: Section where the entry should be set.
 * @entry: Entry to set.
 * @val: Value for the entry.
 * @index: Optional position where the entry will be written.
 *
 * Sets an entry 'entry' to the specified value, and, in case
 * it is not NULL, in the specified section in the configuration file
 * identified by 'cfg'.
 *
 * Returns: 0 on success, -1 otherwise.
 */
int _config_set(config_t *cfg, const char *section, const char *entry, const char *val, unsigned int *index)
{
        int ret;

        if ( section )
                ret = new_section_line(cfg, section, entry, val, index);
        else
                ret = new_entry_line(cfg, entry, val, index);

        if ( ret == 0 )
                cfg->need_sync = TRUE;

        return ret;
}



int _config_del(config_t *cfg, const char *section, const char *entry)
{
        int start;
        char *tmp, *value;
        unsigned int line = 0, end;

        if ( ! entry ) {
                start = search_section(cfg, section, 0);
                if ( start < 0 )
                        return start;

                for ( end = (unsigned int) start + 1; end < cfg->elements && ! is_section(cfg->content[end]); end++ );

                while ( start >= 2 && ! *cfg->content[start - 1] && ! *cfg->content[start - 2] )
                        start--;

        } else {
                start = search_entry(cfg, section, entry, &line, &tmp, &value);
                if ( start < 0 )
                        return start;

                free_val(&tmp);
                free_val(&value);

                end = (unsigned int) start + 1;
        }

        cfg->need_sync = TRUE;
        return op_delete_line(cfg, start, end);
}



static const char *get_variable_content(config_t *cfg, const char *variable)
{
        const char *ptr;
        unsigned int line = 0;

        /*
         * Variable sets at runtime.
         */
        ptr = variable_get(variable);
        if ( ! ptr )
                /*
                 * Other variable (declared in the configuration file).
                 */
                ptr = _config_get(cfg, NULL, variable, &line);

        return ptr;
}




/*
 * _config_get_next:
 * @cfg: Configuration file identifier.
 * @section: Pointer address where the current section should be stored.
 * @entry: Pointer address where the current entry should be stored.
 * @value: Pointer address where the current value should be stored.
 * @line: Pointer to a line number we should start the search at.
 *
 * Parses the whole configuration file starting at @line,
 * and stores the current section, entry and value within the
 * provided argument.
 *
 * The caller has to call config_get_next() until it returns -1
 * or memory will be leaked.
 *
 * If the value gathered starts with a '$', which means it is
 * a variable, the variable is automatically looked up.
 *
 * Returns: 0 on success, -1 if there is nothing more to read.
 */
int _config_get_next(config_t *cfg, char **section, char **entry, char **value, unsigned int *line)
{
        int ret;
        char *ptr;

        free_val(entry);
        free_val(value);
        free_val(section);

        if ( ! cfg->content || *line >= cfg->elements )
                return -1;

        while ( *line < cfg->elements ) {

                ptr = cfg->content[*line];
                ptr += strspn(ptr, " \t\r");
                (*line)++;

                if ( ! *ptr || is_line_commented(ptr) )
                        continue;

                if ( is_section(ptr) )
                        return parse_section_buffer(ptr, section, value, FALSE);

                ret = parse_buffer(ptr, entry, value);
                if ( ret >= 0 && **entry == '$' ) {
                        free_val(entry);
                        free_val(value);
                        continue;
                }

                return ret;
        }

        (*line)--;

        return -1;
}




/**
 * _config_get_section:
 * @cfg: Configuration file identifier.
 * @section: Section we are searching.
 * @line: Pointer to a line number we should start the search at.
 *
 * If @section is found, @line is updated to reflect
 * the line where the section was found.
 *
 * Returns: 0 if the section was found, -1 otherwise.
 */
int _config_get_section(config_t *cfg, const char *section, unsigned int *line)
{
        int ret;

        if ( ! cfg->content )
                return -1;

        ret = search_section(cfg, section, *line);
        if ( ret < 0 )
                return -1;

        *line = ret;

        return 0;
}





/*
 * _config_get:
 * @cfg: Configuration file identifier.
 * @section: Section to gather the entry from.
 * @entry: Entry to gather the value from.
 * @line: Pointer to a line number we should start the search at.
 *
 * Gets value associated with @entry, in the optionaly specified
 * @section, in the configuration file represented by the @cfg
 * abstracted data type. If @entry is found, update @line to reflect
 * the line it was found at.
 *
 * If the value gathered starts with a '$', which means it is
 * a variable, the variable is automatically looked up.
 *
 * If both @entry and @section are NULL, config_get() will try to
 * look up another entry of the same name (and in the same section if
 * section was previously set) as the one previously searched.
 *
 * Returns: The entry value on success, an empty string if the entry
 * exists but has no value, NULL on error.
 */
char *_config_get(config_t *cfg, const char *section, const char *entry, unsigned int *line)
{
        int ret;
        const char *var;
        char *tmp, *value;
        unsigned int index;

        if ( ! cfg->content )
                return NULL;

        index = (*line) ? *line - 1 : 0;

        ret = search_entry(cfg, section, entry, &index, &tmp, &value);
        if ( ret < 0 )
                return NULL;

        *line = index + 1;
        free(tmp);

        /*
         * The requested value point to a variable.
         */
        if ( value[0] == '$' ) {
                var = get_variable_content(cfg, value + 1);
                if ( var ) {
                        free(value);
                        value = strdup(var);
                }
        }

        return value;
}




/**
 * _config_close:
 * @cfg: Configuration file identifier.
 *
 * Close the 'cfg' object, used to access the configuration file.
 * Any change made with the config_set() function call will be written.
 *
 * Returns: 0 on success, -1 otherwise.
 */
int _config_close(config_t *cfg)
{
        int ret = 0;

        if ( cfg->content ) {
                if ( cfg->need_sync )
                        ret = sync_and_free_file_content(cfg);

                if ( ret < 0 || ! cfg->need_sync )
                        free_file_content(cfg);
        }

        free(cfg->filename);
        free(cfg);

        return ret;
}




/**
 * _config_open:
 * @new: Pointer address where to store the new #config_t object.
 * @filename: The configuration file.
 *
 * Opens the configuration file pointed to by 'filename' and loads it into memory,
 * the returned #config_t object will have to be used for any operation on this
 * configuration file.
 *
 * Returns: 0 on success, negative value otherwise.
 */
int _config_open(config_t **new, const char *filename)
{
        int ret;
        config_t *cfg;

        cfg = calloc(1, sizeof(*cfg));
        if ( ! cfg )
                return prelude_error_from_errno(errno);

        cfg->filename = strdup(filename);
        if ( ! cfg->filename ) {
                free(cfg);
                return prelude_error_from_errno(errno);
        }

        ret = load_file_in_memory(cfg);
        if ( ret < 0 ) {
                free(cfg->filename);
                free(cfg);
                return ret;
        }

        *new = cfg;

        return ret;
}
