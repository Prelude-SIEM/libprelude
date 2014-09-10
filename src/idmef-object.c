/*****
*
* Copyright (C) 2014 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@gmail.com>
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include "prelude.h"
#include "idmef-object-prv.h"
#include "idmef-class.h"


idmef_object_t *idmef_object_ref(idmef_object_t *obj)
{
        int ret;

        ret = idmef_class_ref(obj->_idmef_object_id, obj);
        prelude_return_val_if_fail(ret == 0, NULL);

        return obj;
}



void idmef_object_destroy(idmef_object_t *obj)
{
        idmef_class_destroy(obj->_idmef_object_id, obj);
}



int idmef_object_compare(idmef_object_t *obj1, idmef_object_t *obj2)
{
        if ( obj1->_idmef_object_id != obj2->_idmef_object_id )
                return -1;

        return idmef_class_compare(obj1->_idmef_object_id, obj1, obj2);
}



int idmef_object_clone(idmef_object_t *obj, idmef_object_t **dst)
{
        return idmef_class_clone(obj->_idmef_object_id, obj, (void **) dst);
}



int idmef_object_copy(idmef_object_t *src, idmef_object_t *dst)
{
        return idmef_class_copy(src->_idmef_object_id, src, dst);
}



int idmef_object_print(idmef_object_t *obj, prelude_io_t *fd)
{
        return idmef_class_print(obj->_idmef_object_id, obj, fd);
}