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
#include <inttypes.h>
#include <sys/types.h>

#include "common.h"
#include "config-engine.h"
#include "variable.h"
#include "prelude-log.h"


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
 * Remove '\n' and ';' at the end of the string. 
 */
static char *chomp(char *string) 
{
        char *ptr;
        
        if ( (ptr = strrchr(string, ';')) || (ptr = strrchr(string, '\n')) )
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
        while ( *line == ' ' || *line == '\t' )
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
        free(line[0]); *line = nline;
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
        if ( ! cfg->content ) 
                cfg->elements = 1;
        
        cfg->elements++;
        
        cfg->content = prelude_realloc(cfg->content, cfg->elements * sizeof(char **));
        if ( ! cfg->content )
                return -1;        
        
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
                return -1;
        
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
        if ( ! fd ) {
                log(LOG_ERR, "couldn't open %s for reading.\n", cfg->filename);
                return -1;
        }
        
        while ( fgets(line, sizeof(line), fd) ) {
                
                ret = op_append_line(cfg, strdup(chomp(line)));
                if ( ret < 0 ) {
                        fclose(fd);
                        return -1;
                }
                
        }

        fclose(fd);

        return 0;
}




static char *strip_value(const char *in)
{
        char *ret;
        const char *start, *end;
        
        start = in;
        while ( *in == ' ' || *in == '\t' )
                start = ++in;

        end = in + strlen(in) - 1;
        while ( *end == ' ' || *end == '\t' )
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

        ptr = strsep(&buf, "=");
        if ( ! *ptr )
                return -1;
        
        *entry = strip_value(ptr);

        if ( buf ) 
                *(buf - 1) = '=';
        
        ptr = strsep(&buf, "\0");
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



static int search_section(config_t *cfg, const char *section, int line) 
{
        char *ptr;
        int i = line, ret;
        
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
                        const char *entry, int line, char **eout, char **vout) 
{
        int i = line, ret;
        
        if ( ! cfg->content )
                return -1;
        
        if ( section && ! line ) {
                
                i = search_section(cfg, section, line);
                if ( i < 0 )
                        return -1;

                if ( is_line_commented(cfg->content[i]) )
                        return -1;
                        
                i++;
        }
                
        for (; cfg->content[i] != NULL; i++ ) {
                if ( section && is_section(cfg->content[i]) == 0 )
                        return -1;
                
                ret = parse_buffer(cfg->content[i], eout, vout);
                if ( ret < 0 )
                        return -1;

                ret = strcmp(entry, *eout);
                if ( ret == 0 )
                        return i;

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
        const char *ptr;
        
        fd = fopen(filename, "w");
        if ( ! fd ) 
                return -1;
        
        for ( i = 0; content[i] != NULL; i++ ) {                
                fwrite(content[i], 1, strlen(content[i]), fd);

                ptr = strpbrk(content[i], "[# ");
                if ( ! ptr )
                        fwrite(";\n", 1, 2, fd);
                else
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
static int new_entry_line(config_t *cfg, const char *entry, const char *val) 
{
        int line;
        char *eout, *vout;
        
        line = search_entry(cfg, NULL, entry, 0, &eout, &vout);
        if ( line < 0 ) 
                return op_append_line(cfg, create_new_line(entry, val));
        
        free_val(&eout);
        free_val(&vout);
        
        op_modify_line(&cfg->content[line], create_new_line(entry, val));

        return 0;
}




/*
 *
 */
static int new_section_line(config_t *cfg, const char *section,
                            const char *entry, const char *val) 
{
        int line, el;
        char *eout, *vout;
        
        line = search_section(cfg, section, 0);
        if ( line < 0 ) {
                char buf[1024];

                snprintf(buf, sizeof(buf), " \n[%s]", section);
                op_append_line(cfg, strdup(buf));

                return op_append_line(cfg, create_new_line(entry, val));
        }

        el = search_entry(cfg, section, entry, 0, &eout, &vout);
        if ( el < 0 ) 
                return op_insert_line(cfg, create_new_line(entry, val), line + 1);

        free_val(&eout);
        free_val(&vout);
        
        op_modify_line(&cfg->content[el], create_new_line(entry, val));

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
        int ret;
        
        if ( section )
                ret = new_section_line(cfg, section, entry, val);
        else
                ret = new_entry_line(cfg, entry, val);

        if ( ret == 0 )
                cfg->need_sync = 1;

        return ret;
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
                *section = NULL;
        
        if ( ! cfg->content )
                return -1;

        free_val(entry);
        free_val(value);
        
        for ( (*line)++; cfg->content[*line - 1] != NULL; (*line)++ ) {
                
                ptr = cfg->content[*line - 1];
                
                while ( *ptr == ' ' || *ptr == '\t' )
                        ptr++;
                
                if ( ! *ptr || is_line_commented(ptr) )
                        continue;
                
                if ( is_section(ptr) ) {
                        free_val(section);
                        return parse_section_buffer(ptr, section, value);
                } else 
                        return parse_buffer(ptr, entry, value);
        }

        (*line)--;
        free_val(section);
        
        return -1;
}




/**
 * config_get_section:
 * @¢fg: Configuration file identifier.
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
        
        l = search_entry(cfg, section, entry, *line, &tmp, &value);
        if ( l < 0 )
                return NULL;
        
        *line = l;        
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
config_t *config_open(const char *filename) 
{
        int ret;
        config_t *cfg;
        
        cfg = malloc(sizeof(config_t));
        if (! cfg )
                return NULL;

        cfg->filename = strdup(filename);
        cfg->need_sync = 0;
        cfg->content = NULL;
        
        ret = load_file_in_memory(cfg);
        if ( ret < 0 ) 
                return NULL;
        
        return cfg;
}

