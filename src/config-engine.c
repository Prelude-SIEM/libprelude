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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>

#include "libmissing.h"
#include "prelude-inttypes.h"
#include "common.h"
#include "config-engine.h"
#include "variable.h"
#include "prelude-log.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CONFIG_ENGINE
#include "prelude-error.h"


#define DEFAULT_INSERTION_POINT "include"


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
        while (*line == ' ' || *line == '\t')
                line++;
        
        if ( *line == '#' )
                return TRUE;

        return FALSE;
}




/*
 * If line contains a section, returns a pointer to
 * the beginning of the section name.
 */
static prelude_bool_t is_section(const char *line) 
{
        while ( *line == ' ' || *line == '\t' || *line == '\n' )
                line++;
        
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

        if ( start > cfg->elements || end > cfg->elements )
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
        
        cfg->content[cfg->elements - 1] = NULL;
              
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
                
        if ( ! cfg->content ) 
                cfg->elements = 1;

        if ( cfg->elements + 1 == 0 )
                return -1;
        
        cfg->elements++;
        
        cfg->content = _prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return prelude_error_from_errno(errno);        

        
        cfg->content[cfg->elements - 2] = line;
        cfg->content[cfg->elements - 1] = NULL;
              
        return 0;
}




/*
 *
 */
static int op_insert_line(config_t *cfg, char *line, unsigned int lins) 
{
        unsigned int i;

        if ( lins >= cfg->elements || ! line )
                return -1;
        
        cfg->elements++;
        
        cfg->content = _prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if (! cfg->content )
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
                return prelude_error_from_errno(errno);
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
        
        slen = strspn(in, " \"\t");

        elen = tlen - slen;
        if ( ! elen ) {
                *out = strdup("");
                return (*out) ? 0 : prelude_error_from_errno(errno);
        }
        
        elen--;        
        while ( in[elen] == ' ' || in[elen] == '\t' || in[elen] == '\"' ) {
                elen--;
        }
        
        *out = strndup(in + slen, elen + 1);

        return (*out) ? 0 : prelude_error_from_errno(errno);
}



static int parse_buffer(const char *str, char **entry, char **value)
{
        int ret;
        size_t len;
        const char *ptr;
        
        if ( ! *str )
                return 0;
        
        *value = *entry = NULL;
        
        ptr = strchr(str, '=');
        len = (ptr) ? ptr - str : strlen(str);
        
        ret = strip_value(entry, str, len);
        if ( ret < 0 )
                return ret;
        
        if ( ! ptr )
                return 0;
        
        return  strip_value(value, ptr + 1, strlen(ptr + 1));
}


static int parse_section_buffer(const char *buf, char **entry, char **value)
{
        int ret;
        char *ptr;

        buf += strspn(buf, "[");
        
        ptr = strchr(buf, ']');
        if ( ptr )
                *ptr = 0;
        
        ret = parse_buffer(buf, entry, value);
        if ( ptr )
                *ptr = ']';

        if ( ret < 0 )
                return ret;
        
        if ( ! *value )
                *value = strdup("default");
        
        return ret;
}




static int search_section(config_t *cfg, const char *section, unsigned int i) 
{
        int ret;
        char *entry, *value, *wentry, *wvalue;
        
        if ( ! cfg->content )
                return -1;

        ret = parse_section_buffer(section, &wentry, &wvalue);        
        if ( ret < 0 )
                return ret;
                
        for ( ; cfg->content[i] != NULL; i++ ) {
                            
                if ( is_line_commented(cfg->content[i]) )
                        continue;
                
                if ( ! is_section(cfg->content[i]) )
                        continue;
                
                ret = parse_section_buffer(cfg->content[i], &entry, &value);                
                if ( ret < 0 )
                        return ret;
                
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
                
                i = search_section(cfg, section, *index);
                if ( i < 0 )
                        return -1;

                if ( is_line_commented(cfg->content[i]) )
                        return -1;
                        
                i++;
        }
        
        for (; cfg->content[i] != NULL; i++ ) {
                
                if ( section && is_section(cfg->content[i]) ) {
                        *index = 0;
                        return -1;
                }
                
                ret = parse_buffer(cfg->content[i], eout, vout);
                if ( ret < 0 )
                        return -1;
                
                if ( ! *eout )
                        continue;

                ret = strcmp(entry, *eout);
                if ( ret == 0 )
                        return i;
                
                if ( strcmp(*eout, DEFAULT_INSERTION_POINT) == 0 ) 
                        *index = i + 1;
                
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
        
        return (ret < 0 || ret >= len) ? NULL : line;
}



/*
 * Only called if the memory dump of the file was modified,
 * will write the new content to filename 'filename'.
 */
static int sync_and_free_file_content(const char *filename, char **content) 
{
        FILE *fd;
        unsigned int i;
        
        fd = fopen(filename, "w");
        if ( ! fd ) 
                return prelude_error_from_errno(errno);
        
        for ( i = 0; content[i] != NULL; i++ ) {
                
                fwrite(content[i], 1, strlen(content[i]), fd);
                if ( content[i + 1] != NULL )
                        fwrite("\n", 1, 1, fd);
                
                free(content[i]);
        }

        fclose(fd);        
        free(content);
        
        return 0;
}



/*
 * frees the 'content' array, and its pointer.
 * 'content' is the content of the file loaded into memory.
 */
static void free_file_content(char **content) 
{
        unsigned int i;
        
        for (i = 0; content[i] != NULL; i++)
                free(content[i]);

        free(content);
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

        *index = ret;
        op_modify_line(&cfg->content[ret], create_new_line(entry, val));

        return 0;
}




/*
 *
 */
static int new_section_line(config_t *cfg, const char *section,
                            const char *entry, const char *val, unsigned int *index) 
{
        int ret, el;
        char *eout, *vout;
        
        ret = search_section(cfg, section, *index);        
        if ( ret < 0 ) {
                char buf[1024];

                snprintf(buf, sizeof(buf), " \n[%s]", section);

                if ( *index )
                        ret = op_insert_line(cfg, strdup(buf), *index + 1);
                else
                        ret = op_append_line(cfg, strdup(buf));

                if ( ret < 0 )
                        return ret;
                
                if ( ! entry ) {
                        *index = cfg->elements - 2;
                        return 0;
                }
                
                *index = cfg->elements - 1;
                                
                return op_append_line(cfg, create_new_line(entry, val));
        }

        *index = ret;
        if ( ! entry )
                return 0;

        ret++;
        el = search_entry(cfg, section, entry, &ret, &eout, &vout);        
        if ( el < 0 ) 
                return op_insert_line(cfg, create_new_line(entry, val), *index + 1);

        free_val(&eout);
        free_val(&vout);
        
        op_modify_line(&cfg->content[el], create_new_line(entry, val));
        *index = el;
        
        return 0;
}




/**
 * config_set:
 * @cfg: Configuration file identifier.
 * @section: Section where the entry should be set.
 * @entry: Entry to set.
 * @val: Value for the entry.
 *
 * Sets an entry 'entry' to the specified value, and, in case
 * it is not NULL, in the specified section in the configuration file
 * identified by 'cfg'.
 *
 * Returns: 0 on success, -1 otherwise.
 */
int config_set(config_t *cfg, const char *section, const char *entry, const char *val, unsigned int *index) 
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



int config_del(config_t *cfg, const char *section, const char *entry)
{
        char *tmp, *value;
        int start, end, l = 0;
                
        if ( ! entry ) {
                start = search_section(cfg, section, 0);
                if ( start < 0 )
                        return -1;
                
                for ( end = start + 1; cfg->content[end] && ! is_section(cfg->content[end]); end++ );
        } else {
                start = search_entry(cfg, section, entry, &l, &tmp, &value);
                if ( start < 0 )
                        return -1;
                
                end = start + 1;
        }
        
        cfg->need_sync = TRUE;
        
        return op_delete_line(cfg, start, end);
}



static const char *get_variable_content(config_t *cfg, const char *variable) 
{
        int line = 0;
        const char *ptr;
        
        /*
         * Variable sets at runtime.
         */
        ptr = variable_get(variable);
        if ( ! ptr )
                /*
                 * Other variable (declared in the configuration file).
                 */
                ptr = config_get(cfg, NULL, variable, &line);

        return ptr;
}




/*
 * config_get_next:
 * @cfg: Configuration file identifier.
 * @section: Pointer address where the current section should be stored.
 * @entry: Pointer address where the current entry should be stored.
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
int config_get_next(config_t *cfg, char **section, char **entry, char **value, unsigned int *line)
{
        char *ptr;
        
        if ( ! *line ) 
                free_val(section);
        
        if ( ! cfg->content || *line >= cfg->elements )
                return -1;

        free_val(entry);
        free_val(value);
        free_val(section);
        
        for ( (*line)++; cfg->content[*line - 1] != NULL; (*line)++ ) {
                
                ptr = cfg->content[*line - 1];
                
                while ( *ptr == ' ' || *ptr == '\t' )
                        ptr++;
                
                if ( ! *ptr || is_line_commented(ptr) )
                        continue;
                
                if ( is_section(ptr) )
                        return parse_section_buffer(ptr, section, value);
                else          
                        return parse_buffer(ptr, entry, value);
        }

        (*line)--;
        
        return -1;
}




/**
 * config_get_section:
 * @cfg: Configuration file identifier.
 * @section: Section we are searching.
 * @line: Pointer to a line number we should start the search at.
 *
 * If @section is found, @line is updated to reflect
 * the line where the section was found.
 *
 * Returns: 0 if the section was found, -1 otherwise.
 */
int config_get_section(config_t *cfg, const char *section, unsigned int *line) 
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
 * config_get:
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
char *config_get(config_t *cfg, const char *section, const char *entry, unsigned int *line) 
{
        int l;
        const char *var;
        char *tmp, *value;
        
        if ( ! cfg->content )
                return NULL;

        l = (*line) ? *line - 1 : *line;
        
        l = search_entry(cfg, section, entry, &l, &tmp, &value);
        if ( l < 0 )
                return NULL;
        
        *line = l + 1;  
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
 * config_close:
 * @cfg: Configuration file identifier.
 *
 * Close the 'cfg' object, used to access the configuration file.
 * Any change made with the config_set() function call will be written.
 *
 * Returns: 0 on success, -1 otherwise.
 */
int config_close(config_t *cfg) 
{
        int ret = 0;

        if ( cfg->content ) {
                if ( cfg->need_sync )
                        ret = sync_and_free_file_content(cfg->filename, cfg->content);

                if ( ret < 0 || ! cfg->need_sync )
                        free_file_content(cfg->content);
        }
        
        free(cfg->filename);
        free(cfg);
        
        return ret;
}




/**
 * config_open:
 * @filename: The configuration file.
 *
 * Opens the configuration file pointed to by 'filename' and loads it into memory,
 * the returned #config_t object will have to be used for any operation on this
 * configuration file.
 *
 * Returns: a #config_t object on success, NULL otherwise.
 */
int config_open(config_t **new, const char *filename) 
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
