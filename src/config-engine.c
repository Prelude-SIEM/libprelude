/*****
*
* Copyright (C) 2000, 2002, 2003, 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
        int need_sync;  /* do the file need to be synced on disk ? */
        int elements;   /* array number of elements */
};




static void free_val(char **val)
{
        if ( ! *val )
                return;

        free(*val);
        *val = NULL;
}



/*
 * Remove '\n' and at the end of the string. 
 */
static char *chomp(char *string) 
{
        char *ptr;
        
        if ( (ptr = strrchr(string, '\n')) )
                *ptr = '\0';

        return string;
}



static int is_line_commented(const char *line) 
{
        while (*line == ' ' || *line == '\t')
                line++;
        
        if ( *line == '#' )
                return 1;

        return 0;
}




/*
 * If line contain a section, return a pointer to
 * the begining of the section name.
 */
static int is_section(const char *line) 
{
        while ( *line == ' ' || *line == '\t' || *line == '\n' )
                line++;
        
        if ( *line == '[' && strchr(line, ']') )
                return 1;
        
        return 0;
}




static char *get_section(const char *in) 
{
        char *eptr, *ret, old;
        
        in = strchr(in, '[');
        if ( ! in )
                return NULL;
        in++;
        while ( *in == ' ' || *in == '\t' ) in++;
           
        eptr = strchr(in, ']');
        if ( ! eptr )
                return NULL;
        eptr--;
        while (*eptr == ' ' || *eptr == '\t' ) eptr--;
        eptr++;
        
        old = *eptr; *eptr = 0;        
        ret = strdup(in);
        *eptr = old;

        return ret;
}




/*
 * Free old line pointed to by line,
 * and set to the new one.
 */
static void op_modify_line(char **line, char *nline) 
{
        free(*line); *line = nline;
}



static int op_delete_line(config_t *cfg, int start, int end)
{
        int i, j;
        
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
        
        cfg->content = prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return -1;
        
        cfg->content[cfg->elements - 1] = NULL;
              
        return 0;
}



/*
 * Append a line to an array of line.
 * Take the address of the array and the line to append as arguments.
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

        cfg->elements++;
        
        cfg->content = prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return prelude_error_from_errno(errno);        

        
        cfg->content[cfg->elements - 2] = line;
        cfg->content[cfg->elements - 1] = NULL;
              
        return 0;
}




/*
 *
 */
static int op_insert_line(config_t *cfg, char *line, int lins) 
{
        int i;
        
        assert(lins < cfg->elements);

        cfg->elements++;
        
        cfg->content = prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if (! cfg->content )
                return prelude_error_from_errno(errno);
        
        for ( i = cfg->elements - 2; i >= lins; i-- )
                cfg->content[i + 1] = cfg->content[i];
        
        cfg->content[lins] = line;
     
        return 0;
}

        
        

/*
 * Load filename into memory, we use append_line() for that.
 */
static int load_file_in_memory(config_t *cfg)
{
        int ret;
        FILE *fd;
        char line[1024];
        
        fd = fopen(cfg->filename, "r");
        if ( ! fd )
                return prelude_error_from_errno(errno);
        
        while ( fgets(line, sizeof(line), fd) ) {
                ret = op_append_line(cfg, strdup(chomp(line)));
                if ( ret < 0 ) {
                        fclose(fd);
                        return ret;
                }    
        }

        fclose(fd);

        return 0;
}




static char *strip_value(const char *in)
{
        char *ret;
        size_t len;
        const char *start, *end;
        
        start = in;
        while ( *in == ' ' || *in == '\t' )
                start = ++in;

        len = strlen(start);
        end = start + (len ? len - 1 : 0);
        while ( end != start && (*end == ' ' || *end == '\t') )
                end--;
        
        if ( *start == '"' && *end == '"' ) {
                start++;
                end--;
        }
        
        if ( (end - start) < 0 )
             return NULL;
        
        ret = malloc((end - start) + 2);
        if ( ! ret )
                return NULL;

        strncpy(ret, start, end - start + 1);
        ret[end - start + 1] = '\0';
        
        return ret;
}



static int parse_buffer(char *str, char **entry, char **value)
{
        char *ptr, *buf = str;
        
        *value = *entry = NULL;
        
        if ( ! *str )
                return 0;

        ptr = strsep(&buf, "=");        
        if ( ! *ptr )
                return -1;
        
        *entry = strip_value(ptr);

        if ( buf ) 
                *(buf - 1) = '=';
        
        ptr = strsep(&buf, "");
        if ( ptr )
                *value = strip_value(ptr);

        if ( ! *value )
                *value = strdup("");
        
        return 0;
}



static int parse_section_buffer(char *buf, char **entry, char **value)
{
        int ret;
        char *s, *e;

        s = strchr(buf, '[');
        if ( ! s )
                return -1;

        e = strchr(buf, ']');
        if ( ! e )
                return -1;

        *e = 0;
        ret = parse_buffer(s + 1, entry, value);
        *e = ']';
        
        return ret;
}



static int search_section(config_t *cfg, const char *section, int i) 
{
        int ret;
        char *ptr;
        
        if ( ! cfg->content )
                return -1;
        
        for ( ; cfg->content[i] != NULL; i++ ) {
                            
                if ( is_line_commented(cfg->content[i]) )
                        continue;
                
                if ( ! is_section(cfg->content[i]) )
                        continue;
                
                ptr = get_section(cfg->content[i]);
                
                ret = strcasecmp(section, ptr);
                free(ptr);
                
                if ( ret == 0 )
                        return i;
        }

        return -1;
}





/*
 * Search an entry (delimited by '=' character) in content.
 * return the line number matching 'entry' or -1.
 */
static int search_entry(config_t *cfg, const char *section,
                        const char *entry, int *index, char **eout, char **vout) 
{
        int i = *index, insertion_point = 0, ret;
        
        if ( ! cfg->content )
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
                        *index = insertion_point;
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
 * Create a new line using entry 'entry' & value 'val'.
 */
static char *create_new_line(const char *entry, const char *val) 
{
        char *line;
        size_t len = 0;

        if ( ! entry )
                return NULL;
        
        if ( val )
                len = strlen(val) + 3;
        else
                len = 2;

        line = (char *) malloc(strlen(entry) + len);
        if (! line )
                return NULL;

        if ( val )
                sprintf(line, "%s=%s", entry, val);
        else
                sprintf(line, "%s", entry);
        
        return line;
}



/*
 * Only called if the memory dump of the file was modified,
 * will write the new content to filename 'filename'.
 */
static int sync_and_free_file_content(const char *filename, char **content) 
{
        int i;
        FILE *fd;
        
        fd = fopen(filename, "w");
        if ( ! fd ) 
                return prelude_error_from_errno(errno);
        
        for ( i = 0; content[i] != NULL; i++ ) {                
                fwrite(content[i], 1, strlen(content[i]), fd);
                fwrite("\n", 1, 1, fd);
                
                free(content[i]);
        }

        fclose(fd);        
        free(content);

        return 0;
}



/*
 * free the 'content' array, and it's pointer.
 * 'content' is the content of the file loaded into memory.
 */
static void free_file_content(char **content) 
{
        int i;
        
        for (i = 0; content[i] != NULL; i++)
                free(content[i]);

        free(content);
}



/*
 *
 */
static int new_entry_line(config_t *cfg, const char *entry, const char *val, int *index) 
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
                            const char *entry, const char *val, int *index) 
{
        int ret, el;
        char *eout, *vout;
        
        ret = search_section(cfg, section, *index);        
        if ( ret < 0 ) {
                char buf[1024];

                snprintf(buf, sizeof(buf), " \n[%s]", section);
                op_append_line(cfg, strdup(buf));

                *index = cfg->elements - 2;

                if ( ! entry )
                        return 0;
                
                *index = cfg->elements - 1;
                                
                return op_append_line(cfg, create_new_line(entry, val));
        }

        *index = ret;
        if ( ! entry )
                return 0;

        ret++;
        el = search_entry(cfg, section, entry, &ret, &eout, &vout);        
        if ( el < 0 ) 
                return op_insert_line(cfg, create_new_line(entry, val), ret);

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
 * Set an entry 'entry' to the specified value, and, in case
 * it is not NULL, in the specified section in the config file
 * identified by 'cfg'.
 *
 * Returns: 0 on success, -1 otherwise.
 */
int config_set(config_t *cfg, const char *section, const char *entry, const char *val) 
{
        int ret, index = 0;
        
        if ( section )
                ret = new_section_line(cfg, section, entry, val, &index);
        else
                ret = new_entry_line(cfg, entry, val, &index);
        
        if ( ret == 0 ) 
                cfg->need_sync = 1;
        
        return ret;
}



int config_del(config_t *cfg, const char *section, const char *entry)
{
        char *tmp, *value;
        int start, end, l = 0;
        
        if ( ! entry ) {
                start = search_section(cfg, section, 0);
                for ( end = start + 1; cfg->content[end] && ! is_section(cfg->content[end]); end++ );
        } else {
                start = search_entry(cfg, section, entry, &l, &tmp, &value);
	        end = start + 1;
        }
	
        if ( start < 0 )
                return -1;
        
        cfg->need_sync = 1;
        
        return op_delete_line(cfg, start, end);
}



static const char *get_variable_content(config_t *cfg, const char *variable) 
{
        int line = 0;
        const char *ptr;
        
        /*
         * Variable set at runtime.
         */
        ptr = variable_get(variable);
        if ( ! ptr )
                /*
                 * other variable (declared in the configuration file).
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
 * Parse the whole configuration file starting at @line,
 * and store the current section, entry and value within the
 * provided argument.
 *
 * The caller has to call config_get_next() until it return -1
 * or memory will be leaked.
 *
 * If the value gathered start with a '$', which mean it is
 * a variable, the variable is automatically looked up.
 *
 * Returns: 0 on success, -1 if there is nothing more to read.
 */
int config_get_next(config_t *cfg, char **section, char **entry, char **value, int *line)
{
        char *ptr;

        if ( ! *line ) 
                free_val(section);
        
        if ( ! cfg->content )
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
                
                if ( is_section(ptr) ) {
                        return parse_section_buffer(ptr, section, value);
                } else {                        
                        return parse_buffer(ptr, entry, value);
                }
        }

        (*line)--;
        
        return -1;
}




/**
 * config_get_section:
 * @cfg: Configuration file identifier.
 * @section: Section we are searching for.
 * @line: Pointer to a line number we should start the search at.
 *
 * If @section is found, @line is updated to reflect
 * the line where the section was found.
 *
 * Returns: 0 if the section was found, -1 otherwise.
 */
int config_get_section(config_t *cfg, const char *section, int *line) 
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
 * Get value associated with @entry, in the optionnaly specified
 * @section, in the configuration file represented by the @cfg
 * abstract data type. If @entry is found, update @line to reflect
 * the line it was found at.
 *
 * If the value gathered start with a '$', which mean it is
 * a variable, the variable is automatically looked up.
 *
 * If both @entry and @section are NULL, config_get() will try to
 * look up another entry of the same name (and in the same section if
 * section was previously set) as the one previously searched.
 *
 * Returns: The entry value on success, an empty string if the entry
 * exist but have no value, NULL on error.
 */
char *config_get(config_t *cfg, const char *section, const char *entry, int *line) 
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
 * Open the configuration file pointed to by 'filename' and load it into memory,
 * the returned #config_t object will have to be used for any operation on this
 * configuration file.
 *
 * Returns: a #config_t object on success, NULL otherwise.
 */
int config_open(config_t **new, const char *filename) 
{
        int ret;
        config_t *cfg;
        
        cfg = malloc(sizeof(config_t));
        if ( ! cfg )
                return prelude_error_from_errno(errno);

        cfg->filename = strdup(filename);
        if ( ! cfg->filename ) {
                free(cfg);
                return prelude_error_from_errno(errno);
        }
        
        cfg->need_sync = 0;
        cfg->content = NULL;

        ret = load_file_in_memory(cfg);
        if ( ret < 0 ) {
                free(cfg->filename);
                free(cfg);
                return ret;
        }
        
        *new = cfg;
        
        return ret;
}
