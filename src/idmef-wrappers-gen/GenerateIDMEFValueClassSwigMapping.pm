# Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
# Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
#
# This file is part of the Prelude library.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

package GenerateIDMEFValueClassSwigMapping;

use Generate;
@ISA = qw/Generate/;

use strict;
use IDMEFTree; 


sub	header
{
    my	$self = shift;

    $self->output("
/*****
*
* Copyright (C) 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v\@prelude-ids.com>
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

/* Auto-generated by the GenerateIDMEFValueClassSwigMapping package */
#include \"idmef-value.h\"
#include \"idmef-tree-wrap.h\"

");
}



sub	footer
{
    my	$self = shift;
    my	$tree = shift;
    my	$last_id = -1;
    my  $object_class;

    $self->output("
void *swig_idmef_value_get_descriptor(idmef_value_t *value)
{
        unsigned int i = 0;
        void *object = idmef_value_get_object(value);
        idmef_class_id_t wanted_class = idmef_value_get_class(value);
	const struct {
	        idmef_class_id_t class;
	        void *swig_type;
	} tbl[] = {
");
    
    foreach my $obj ( sort { $a->{id} <=> $b->{id} } map { 
                      ($_->{obj_type} != &OBJ_PRE_DECLARED ? $_ : () ) } @{ $tree->{obj_list} } ) {

       if ( $obj->{obj_type} == &OBJ_STRUCT ) { 
           $object_class = "IDMEF_CLASS_ID_" . uc("$obj->{short_typename}");
           $self->output("                { $object_class, SWIGTYPE_p_$obj->{typename} },\n");
       }
    }
  
    $self->output("                { 0, NULL }
        };

        for ( i = 0; tbl[i].swig_type != NULL; i++ ) {
                if ( tbl[i].class == wanted_class )
		        return tbl[i].swig_type;
        }

        return NULL;
}
");

}

1;
