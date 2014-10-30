# Copyright (C) 2003-2012 CS-SI. All Rights Reserved.
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
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

package GenerateIDMEFMessageReadC;

use Generate;
@ISA = qw/Generate/;

use strict;
use IDMEFTree;

sub     header
{
     my $self = shift;

     $self->output("
/*****
*
* Copyright (C) 2001-2012 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v\@prelude-ids.com>
* Author: Nicolas Delon <nicolas.delon\@prelude-ids.com>
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

/* Auto-generated by the GenerateIDMEFMessageReadC package */
#include \"config.h\"

#include <stdio.h>
#include <unistd.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_MESSAGE_READ
#include \"prelude-error.h\"
#include \"prelude-inttypes.h\"
#include \"prelude-list.h\"
#include \"prelude-extract.h\"
#include \"prelude-io.h\"
#include \"idmef-message-id.h\"
#include \"idmef.h\"
#include \"idmef-tree-wrap.h\"

#include \"idmef-message-read.h\"

#define prelude_extract_string_safe(out, buf, len, msg) extract_string_safe_f(__FUNCTION__, __LINE__, out, buf, len)

static inline int extract_string_safe_f(const char *f, int line, prelude_string_t **out, char *buf, size_t len)
\{
        int ret;

        /*
         * we use len - 1 since len is supposed to include \\0 to avoid making a dup.
         */
        ret = prelude_string_new_ref_fast(out, buf, len - 1);
        if ( ret < 0 )
                ret = prelude_error_verbose(prelude_error_get_code(ret), \"%s:%d could not extract IDMEF string: %s\", f, line, prelude_strerror(ret));

        return ret;
\}


static inline int prelude_extract_time_safe(idmef_time_t **out, void *buf, size_t len, prelude_msg_t *msg)
\{
        int ret;

        /*
         * sizeof(sec) + sizeof(usec) + sizeof(gmt offset).
         */
        if ( len != 12 )
                return prelude_error_make(PRELUDE_ERROR_SOURCE_EXTRACT, PRELUDE_ERROR_INVAL_IDMEF_TIME);

        ret = idmef_time_new(out);
        if ( ret < 0 )
                return ret;

        idmef_time_set_sec(*out, prelude_extract_uint32(buf));
        idmef_time_set_usec(*out, prelude_extract_uint32((unsigned char *) buf + 4));
        idmef_time_set_gmt_offset(*out, prelude_extract_int32((unsigned char *) buf + 8));

        return 0;
\}


static inline int prelude_extract_data_safe(idmef_data_t **out, void *buf, uint32_t len, prelude_msg_t *msg)
\{
        int ret;
        uint8_t tag;
        idmef_data_type_t type = 0;

        ret = prelude_extract_uint32_safe(&type, buf, len);
        if ( ret < 0 )
                return ret;

        ret = prelude_msg_get(msg, &tag, &len, &buf);
        if ( ret < 0 )
                return ret;

        *out = NULL;

        switch ( type ) \{
        case IDMEF_DATA_TYPE_CHAR: \{
                uint8_t tmp = 0;

                ret = prelude_extract_uint8_safe(&tmp, buf, len);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_char(out, (char) tmp);
                break;
        \}

        case IDMEF_DATA_TYPE_BYTE: \{
                uint8_t tmp = 0;

                ret = prelude_extract_uint8_safe(&tmp, buf, len);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_byte(out, tmp);
                break;
        \}

        case IDMEF_DATA_TYPE_UINT32: \{
                uint32_t tmp = 0;

                ret = prelude_extract_uint32_safe(&tmp, buf, len);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_int(out, tmp);
                break;
        \}

        case IDMEF_DATA_TYPE_INT: \{
                uint64_t tmp = 0;

                ret = prelude_extract_uint64_safe(&tmp, buf, len);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_int(out, tmp);
                break;
        \}

        case IDMEF_DATA_TYPE_FLOAT: \{
                float tmp = 0;

                ret = prelude_extract_float_safe(&tmp, buf, len);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_float(out, tmp);
                break;
        \}

        case IDMEF_DATA_TYPE_BYTE_STRING: \{
                ret = idmef_data_new_ptr_ref_fast(out, type, buf, len);
                break;
        \}

        case IDMEF_DATA_TYPE_CHAR_STRING: \{
                const char *tmp = NULL;

                ret = prelude_extract_characters_safe(&tmp, buf, len);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_ptr_ref_fast(out, type, tmp, len);
                break;
        \}

        case IDMEF_DATA_TYPE_TIME: \{
                idmef_time_t *time;

                ret = prelude_extract_time_safe(&time, buf, len, msg);
                if ( ret < 0 )
                        return ret;

                ret = idmef_data_new_time(out, time);
                break;
        \}

        case IDMEF_DATA_TYPE_UNKNOWN:
                /* nop */;
        \}

        return ret;
\}


");
}

sub     struct_field_normal
{
    my  $self = shift;
    my  $tree = shift;
    my  $struct = shift;
    my  $field = shift;
    my  $ptr = ($field->{metatype} & (&METATYPE_STRUCT | &METATYPE_LIST)) ? "*" : "";
    my  $init = ($field->{metatype} & (&METATYPE_STRUCT | &METATYPE_LIST)) ? "NULL" : "0";
    my  $type = shift || $field->{value_type};
    my  $var_type = shift || "$field->{typename}";
    my  $extra_msg = "";

    $extra_msg = ", msg" if ( $field->{metatype} & (&METATYPE_STRUCT | &METATYPE_LIST) );

    $self->output("
                        case IDMEF_MSG_",  uc($struct->{short_typename}), "_", uc($field->{short_name}), ": \{
                                ${var_type} ${ptr}tmp = ${init};

                                ret = prelude_extract_${type}_safe(&tmp, buf, len${extra_msg});
                                if ( ret < 0 )
                                        return ret;
");

    if ( $field->{metatype} & &METATYPE_LIST ) {
           $self->output("
                                idmef_$struct->{short_typename}_set_$field->{short_name}($struct->{short_typename}, tmp, -1);
                                break;
                        \}
"); } else {
           $self->output("
                                idmef_$struct->{short_typename}_set_$field->{short_name}($struct->{short_typename}, tmp);
                                break;
                        \}
"); }
}

sub     struct_field_struct
{
    my  $self = shift;
    my  $tree = shift;
    my  $struct = shift;
    my  $field = shift;
    my  $name = shift || $field->{name};

    $self->output("
                        case IDMEF_MSG_",  uc($field->{short_typename}), "_TAG", ": \{
                                int ret;
                                $field->{typename} *tmp = NULL;
");

    if ( $field->{metatype} & &METATYPE_LIST ) {
           $self->output("
                                ret = idmef_$struct->{short_typename}_new_${name}($struct->{short_typename}, &tmp, -1);
                                if ( ret < 0 )
                                        return ret;
");
    } else {
           $self->output("
                                ret = idmef_$struct->{short_typename}_new_${name}($struct->{short_typename}, &tmp);
                                if ( ret < 0 )
                                        return ret;
");
    }

    $self->output("


                                ret = idmef_$field->{short_typename}_read(tmp, msg);
                                if ( ret < 0 )
                                        return ret;

                                break;
                        \}
");
}

sub     struct_field_union
{
    my  $self = shift;
    my  $tree = shift;
    my  $struct = shift;
    my  $field = shift;

    foreach my $member ( @{$field->{member_list}} ) {
        $self->struct_field_struct($tree, $struct, $member);
    }
}

sub     struct
{
    my  $self = shift;
    my  $tree = shift;
    my  $struct = shift;

    $self->output("
/**
 * idmef_$struct->{short_typename}_read:
 * \@$struct->{short_typename}: Pointer to a #$struct->{typename} object.
 * \@msg: Pointer to a #prelude_msg_t object, containing a message.
 *
 * Read an idmef_$struct->{short_typename} from the \@msg message, and
 * store it into \@$struct->{short_typename}.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_$struct->{short_typename}_read($struct->{typename} *$struct->{short_typename}, prelude_msg_t *msg)
\{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t len;

        while ( 1 ) \{
                ret = prelude_msg_get(msg, &tag, &len, &buf);
                if ( ret < 0 )
                        return ret;

                switch ( tag ) \{
");

    foreach my $field ( @{$struct->{field_list}} ) {

        if ( $field->{metatype} & &METATYPE_NORMAL ) {

            if ( $field->{metatype} & &METATYPE_PRIMITIVE ) {
                $self->struct_field_normal($tree, $struct, $field);

            } elsif ( $field->{metatype} & &METATYPE_ENUM ) {
                $self->struct_field_normal($tree, $struct, $field, "int32", "int32_t");

            } else {
                $self->struct_field_struct($tree, $struct, $field);
            }

        } elsif ( $field->{metatype} & &METATYPE_LIST ) {
            if ( $field->{metatype} & &METATYPE_PRIMITIVE ) {
                $self->struct_field_normal($tree, $struct, $field);

            } else {
                $self->struct_field_struct($tree, $struct, $field, $field->{short_name});
            }

        } elsif ( $field->{metatype} & &METATYPE_UNION ) {
            $self->struct_field_union($tree, $struct, $field);
        }
    }

    $self->output("
                        case IDMEF_MSG_END_OF_TAG:
                                return 0;

                        default:
                                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_UNKNOWN_TAG, \"Unknown tag while reading " . $struct->{typename} . ": '%u'\", tag);
                \}

        \}

        return 0;
\}
");
}

1;
