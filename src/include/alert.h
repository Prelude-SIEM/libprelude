/*****
*
* Copyright (C) 1998 - 2000 Vandoorselaere Yoann
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

#ifndef ALERT_H
#define ALERT_H

#include <inttypes.h>
#include "list.h"
#include "plugin-common.h"
#include "timer.h"


typedef enum { normal, maybe_faked, maybe_not_reliable, guess } rkind_t;

typedef struct {
        /*
         * This is only used to add
         * the alert structure into the async IO queue.
         */
        struct list_head list;
        
        /*
         * DetectPlugins infos.
         */
        plugin_generic_t *plugin;
        
        char *quickmsg;
        size_t quickmsglen;

        char message[1024];
        size_t messagelen;
        
        rkind_t kind;
        unsigned int count;
        
        time_t time_start;
        time_t time_end;
        
        /*
         * Received packet.
         */
        uint8_t sensor_data_id;
        void *sensor_data;
} alert_t;


#define alert_plugin(alert) (alert)->plugin

#define alert_quickmsg(alert) (alert)->quickmsg

#define alert_quickmsg_len(alert) (alert)->quickmsglen

#define alert_message(alert) (alert)->message

#define alert_message_len(alert) (alert)->messagelen

#define alert_kind(alert) (alert)->kind

#define alert_count(alert) (alert)->count

#define alert_time_start(alert) (alert)->time_start

#define alert_time_end(alert) (alert)->time_end

#define alert_data(alert) (alert)->data


#endif




