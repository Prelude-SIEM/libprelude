# Copyright (C) 2016-2020 CS GROUP - France. All Rights Reserved.
# Author: Yoann Vandoorselaere <yoannv@gmail.com>
#
# This file is part of the Prelude library.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

package GenerateIDMEFMessagePrintJSONH;

use Generate;
@ISA = qw/Generate/;

use strict;
use IDMEFTree;

sub header
{
    my  $self = shift;

    $self->output("
/*****
*
* Copyright (C) 2016-2020 CS GROUP - France. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv\@gmail.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

/* Auto-generated by the GenerateIDMEFMessagePrintJSONH package */

#ifndef _LIBPRELUDE_IDMEF_MESSAGE_PRINT_JSON_H
#define _LIBPRELUDE_IDMEF_MESSAGE_PRINT_JSON_H

#ifdef __cplusplus
 extern \"C\" \{
#endif

");
}

sub struct
{
    my  $self = shift;
    my  $tree = shift;
    my  $struct = shift;

    $self->output("
int idmef_$struct->{short_typename}_print_json($struct->{typename} *ptr, prelude_io_t *fd);");
}

sub footer
{
    my  $self = shift;
    my  $tree = shift;

    $self->output("

int idmef_message_print_json(idmef_message_t *ptr, prelude_io_t *fd);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_IDMEF_MESSAGE_PRINT_JSON_H */
"
);
}

1;
