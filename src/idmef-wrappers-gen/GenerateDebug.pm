# Copyright (C) 2003-2015 CS-SI. All Rights Reserved.
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

package GenerateDebug;

use strict;
use IDMEFTree; # import constants

sub	new
{
    bless({ }, shift);
}

# sub	obj
# {
#     my	$self = shift;
#     my	$tree = shift;
#     my	$obj = shift;

#     print "obj obj_type:$obj->{obj_type} type:$obj->{type}\n";
# }

sub	struct
{
    my	$self = shift;
    my	$tree = shift;
    my	$struct = shift;

#     print "struct $struct->{type} $struct->{id}\n";
#     foreach my $field ( @{ $struct->{field_list} } ) {
# 	if ( $field->{metatype} == &METATYPE_NORMAL ) {
# 	    print "\t", "$field->{type} $field->{name} (", $field->{ptr} ? "pointer" : "non-pointer", ")\n";
	    
# 	} elsif ( $field->{metatype} == &METATYPE_LIST ) {
# 	    print "\t", "list of $field->{type} named $field->{name}\n";

# 	} elsif ( $field->{metatype} == &METATYPE_UNION) {
# 	    print "\t", "union $field->{name} switched on variable $field->{var}\n";
# 	    foreach my $member ( @{ $field->{member_list} } ) {
# 		print("\t\t",
# 		      "member $member->{type} $member->{name} switched on value $member->{value} (",
# 		      $member->{ptr} ? "pointer" : "non-pointer",
# 		      ")\n");
# 	    }
# 	}
#     }

    print "/*\n";
    print " * $_\n" foreach ( @{ $struct->{desc} } );
    print " */\n\n";
}

sub	enum
{
    my	$self = shift;
    my	$tree = shift;
    my	$enum = shift;

#     print "enum $enum->{type} $enum->{id}\n";
#     foreach my $field ( @{ $enum->{field_list} } ) {
# 	print "\t", "$field->{name}: $field->{value}\n";
#     }

    print "/*\n";
    print " * $_\n" foreach ( @{ $enum->{desc} } );
    print " */\n\n";
}

sub	forced
{
    my	$self = shift;
    my	$tree = shift;
    my	$forced = shift;

#     print "forced $forced->{type} $forced->{class}\n\n";
}

1;
