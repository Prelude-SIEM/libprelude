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
#include <errno.h>
#include <assert.h>

#include "config-engine.h"
#include "common.h"


struct config {
        char *filename; /* filename for this session */
        char **content; /* content of the file */
        int need_sync;  /* do the file need to be synced on disk ? */
        int elements;   /* array number of elements */
};



/*
 * Remove '\n' at the end of the string. 
 */
static char *chomp(char *string) 
{
        char *ptr;
        
        if ( (ptr = strchr(string, ';')) || (ptr = strchr(string, '\n')) )
                *ptr = '\0';
        
        return string;
}




/*
 * If line contain a section, return a pointer to
 * the begining of the section name.
 */
static int is_section(const char *line) 
{

        if ( strchr(line, '[') && strchr(line, ']') )
                return 0;
        return -1;
}




/*
 *
 */
static int cmp_entry(char *string, const char *wanted) 
{
        int ret;
        char old;
        char *ptr;

        /*
         * Locate the last character before the value.
         */
        ptr = strrchr(string, '=');
        if ( ! ptr )
                ptr = string + strlen(string);
        else
                ptr--;
        
        while ( ptr != string && *ptr == ' ' )
                ptr--;

        ptr++;
        
        old = *ptr; *ptr = 0;

        ret = strcmp(string, wanted);
        
        *ptr = old;
        
        return ret;
}



/*
 *
 */
static int cmp_section(const char *string, const char *wanted) 
{
        int ret;
        const char *ptr;
        char *eptr, old;
        
        ptr = strchr(string, '[');
        if (!ptr)
                return -1;
        while ( *++ptr == ' ' );
           
        eptr = strchr(string, ']');
        if (!eptr)
                return -1;
        while (*--eptr == ' ');
        eptr++;
        
        old = *eptr; *eptr = 0;        
        ret = strcmp(ptr, wanted);
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
        
        cfg->content = (char **) realloc(cfg->content, cfg->elements * sizeof(char **));
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
        
        cfg->content = (char **) realloc(cfg->content, cfg->elements * sizeof(char **));
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
        if (! fd && errno == ENOENT ) 
                return 0;
        
        while ( fgets(line, sizeof(line), fd) ) {
                
                ret = op_append_line(cfg, strdup(chomp(line)));
                if ( ret < 0 )
                        return -1;
                
        }

        fclose(fd);

        return 0;
}




static int search_section(config_t *cfg, const char *section) 
{
        int i;
        
        if ( ! cfg->content )
                return -1;
        
        for ( i = 0; cfg->content[i] != NULL; i++ ) {
                if ( cmp_section(cfg->content[i], section) == 0 )
                        return i;
        }

        return -1;
}





/*
 * Search an entry (delimited by '=' character) in content.
 * return the line number matching 'entry' or -1.
 */
static int search_entry(config_t *cfg, const char *section, const char *entry) 
{
        int i = 0;

        if ( ! cfg->content )
                return -1;
        
        if ( section ) {
                i = search_section(cfg, section);
                if ( i < 0 )
                        return -1;

                i++;
        }
        
        for (; cfg->content[i] != NULL; i++ ) {
                if ( section && is_section(cfg->content[i]) == 0 )
                        return -1;
                
                if ( cmp_entry(cfg->content[i], entry) == 0 ) 
                        return i;
        }
        

        return -1;
}



/*
 * Create a new line using entry 'entry' & value 'val'.
 */
static char *create_new_line(const char *entry, const char *val) 
{
        char *line;
        int len = 0;

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

        line = search_entry(cfg, NULL, entry);
        if ( line < 0 ) 
                return op_append_line(cfg, create_new_line(entry, val));

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

        line = search_section(cfg, section);
        if ( line < 0 ) {
                char buf[1024];

                snprintf(buf, sizeof(buf), " \n[%s]", section);
                op_append_line(cfg, strdup(buf));

                return op_append_line(cfg, create_new_line(entry, val));
        }

        el = search_entry(cfg, section, entry);
        if ( el < 0 ) 
                return op_insert_line(cfg, create_new_line(entry, val), line + 1);
        
        op_modify_line(&cfg->content[el], create_new_line(entry, val));

        return 0;
}




/*
 * Set an entry 'entry' to the value 'value',
 * and in section 'section' if it is no NULL,
 * to the config file configured in the 'cfg'
 * abstract data type.
 *
 * The entry and section are created if it do not exist.
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



/*
 * Get value for the entry 'entry', in the config file
 * configured in the 'cfg' abstract data type.
 *
 * If section is not NULL, entry will be searched only
 * in this section.
 */
const char *config_get(config_t *cfg, const char *section, const char *entry) 
{
        int line;
        char *ret, *p;
        
        if ( ! cfg->content )
                return NULL;
        
        line = search_entry(cfg, section, entry);
        if ( line < 0 )
                return NULL;

        ret = strchr(cfg->content[line], '=');
        if ( ! ret )
                return "";

        ret++;

        /*
         * Strip trailling white space.
         */
        while ( *ret == ' ' ) ret++;
        for ( p = ret + strlen(ret); p && *p == ' '; p-- )
                *p = 0;

        return ret;
}



/*
 * Close the 'cfg' object,
 * used to access the configuration file.
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




/*
 * Open 'filename', and load it into memory, return
 * a config_t structure which is an abstract for
 * config file operation.
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




