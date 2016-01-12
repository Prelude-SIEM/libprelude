# Copyright (C) 2003-2016 CS-SI. All Rights Reserved.
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

package IDMEFTree;

BEGIN
{
    use Exporter;

    @ISA = qw/Exporter/;
    @EXPORT = qw(METATYPE_PRIMITIVE METATYPE_OPTIONAL_INT METATYPE_STRUCT
		 METATYPE_ENUM METATYPE_NORMAL METATYPE_LIST METATYPE_KEYED_LIST METATYPE_UNION
		 OBJ_STRUCT OBJ_ENUM OBJ_PRE_DECLARED);
}

use	strict;

sub	METATYPE_PRIMITIVE		{ 0x01 }
sub	METATYPE_OPTIONAL_INT		{ 0x02 }
sub	METATYPE_STRUCT			{ 0x04 }
sub	METATYPE_ENUM			{ 0x08 }
sub	METATYPE_NORMAL			{ 0x10 }
sub	METATYPE_LIST			{ 0x20 }
sub	METATYPE_KEYED_LIST             { 0x40 }
sub	METATYPE_UNION			{ 0x80 }

sub	OBJ_STRUCT			{ 0 }
sub	OBJ_ENUM			{ 1 }
sub	OBJ_PRE_DECLARED		{ 2 }

sub	get_idmef_name
{
    my	$name = shift;

    $name =~ s/^idmef_//;
    $name =~ s/_t$//;

    return $name;
}

sub	get_value_type
{
    my	$type = shift;

    $type =~ s/^[^_]+_//;
    $type =~ s/_t$//;

    return $type;
}

sub	debug
{
    (shift)->{debug} and print @_;
}

sub	new
{
    my	$class = shift;
    my	%opt = @_;
    my	$self = { };

    $self->{debug} = $opt{-debug} || 0;
    $self->{filename} = $opt{-filename};

    $self->{lineno} = 0;
    $self->{primitives} = { };
    $self->{objs} = { };
    $self->{obj_list} = [ ];
    $self->{structs} = { };
    $self->{struct_list} = [ ];
    $self->{enums} = { };
    $self->{enum_list} = [ ];
    $self->{pre_declareds} = { };
    $self->{pre_declared_list} = [ ];

    bless($self, $class);

    return $self;
}

sub	load
{
    my	$self = shift;
    my	$file;
    my	$line;

    unless ( open(FILE, "cpp -D_GENERATE -E $self->{filename} |") ) {
	$self->debug("Cannot process file $self->{filename}: $!\n");
	return 0;
    }

    $self->{file} = \*FILE;

    $self->parse;

    close FILE;
    undef $self->{file};

    return 1;
}

sub	get_line
{
    my	$self = shift;
    my	$file = $self->{file};
    my	$line;
    my	$line2;

    defined($line = <$file>) or return undef;
    chomp $line;
    $self->{lineno}++;
    return $line;
}

sub	parse
{
    my	$self = shift;
    my	$line;
    my	$struct;

    while ( defined($line = $self->get_line) ) {
	
	if ( $line =~ /^\s*struct\s+.*\{/ ) {
	    $self->debug("parse struct\n");
	    $self->parse_struct($line);

	} elsif ( $line =~ /^\s*ENUM\(\w*\)/ ) {
	    $self->debug("parse enum\n");
	    $self->parse_enum($line);

	} elsif ( $line =~ /^\s*PRE_DECLARE/ ) {
	    $self->debug("parse register\n");
	    $self->parse_pre_declared($line);

	} elsif ( $line =~ /^\s*PRIMITIVE_TYPE\(/) {
	    $self->debug("parse primitive type\n");
	    $self->parse_primitive($line);
	}

	elsif ( $line =~ /^\s*PRIMITIVE_TYPE_STRUCT\(/ ) {
	    $self->debug("parse primitive type struct\n");
	    $self->parse_primitive_struct($line);
	}
    }

    $struct = @{ $self->{struct_list} }[-1];
    $struct->{toplevel} = 1;
}

sub	parse_struct
{
    my	$self = shift;
    my	$line = shift;
    my	$struct = { obj_type => &OBJ_STRUCT, toplevel => 0, is_listed => 0, is_key_listed => 0, refcount => 0, desc => [ $line ] };
    my	@field_list;
    my	$ptr;
    my	$typename;
    my	$name;
    my  $key;
    my	$id;
    my	$var;

    while ( defined($line = $self->get_line) ) {
	push(@{ $struct->{desc} }, $line);
	if ( $line =~ /^\s*HIDE\(/ ) {
	    next;

	} elsif ( $line =~ /^\s*REFCOUNT\s*\;\s*$/ ) {
	    $struct->{refcount} = 1;
	    $self->debug("struct is refcounted\n");

	} elsif ( $line =~ /^\s*IDMEF_LINKED_OBJECT\s*\;\s*$/ ) {
	    $struct->{is_listed} = 1;
	    $self->debug("struct is listed\n");
	} elsif ( ($typename, $ptr, $name) = $line =~ /^\s*struct\s+(\w+)\s+(\**)(\w+)\;$/ ) {
	    push(@field_list, { metatype => &METATYPE_NORMAL | &METATYPE_STRUCT,
				typename => $typename . "_t",
				short_typename => get_idmef_name($typename),
				name => $name,
				short_name => $name,
				ptr => $ptr ? 1 : 0,
				dynamic_ident => 0 });

	    $self->debug("parse direct struct field metatype:normal name:$name typename:$typename ptr:", $ptr ? "yes" : "no", "\n");

	} 

        elsif ( ($typename, $ptr, $name) = $line =~ /^\s*IGNORED\((\w+)\s*,\s*(\**)(\w+)\)\;/ ) {
	}

        elsif ( ($typename, $ptr, $name) = $line =~ /^\s*REQUIRED\((\w+)\s*,\s*(\**)(\w+)\)\;/ ) {
            my $metatype;
            my $short_typename;
            my $value_type;

            if ( defined $self->{primitives}->{$typename} ) {
                $metatype = $self->{primitives}->{$typename};
                $short_typename = $typename;
                $short_typename =~ s/_t$//;
                $value_type = get_value_type($short_typename);

            } elsif ( defined $self->{structs}->{$typename} ) {
                $metatype = &METATYPE_STRUCT;
                $short_typename = get_idmef_name($typename);

            } elsif ( defined $self->{enums}->{$typename} ) {
                $metatype = &METATYPE_ENUM;
                $short_typename = get_idmef_name($typename);

            } elsif ( defined $self->{pre_declareds}->{$typename} ) {
                $metatype = $self->{pre_declareds}->{$typename}->{metatype};
                $short_typename = get_idmef_name($typename);

            }

            push(@field_list, { metatype => &METATYPE_NORMAL | $metatype,
                                typename => $typename,
                                short_typename => $short_typename,
                                value_type => $value_type,
                                name => $name,
                                short_name => $name,
                                ptr => ($ptr ? 1 : 0),
				required => 1,
                                dynamic_ident => 0 });

            $self->debug("parse struct field metatype:normal name:$name typename:$typename ptr:", ($ptr ? 1 : 0) ? "yes" : "no", "\n");

        } 

	elsif ( ($typename, $ptr, $name) = $line =~ /^\s*(\w+)\s+(\**)(\w+)\;/ ) {
	    my $metatype;
	    my $short_typename;
	    my $value_type;

	    if ( defined $self->{primitives}->{$typename} ) {
		$metatype = $self->{primitives}->{$typename};
		$short_typename = $typename;
		$short_typename =~ s/_t$//;
		$value_type = get_value_type($short_typename);

	    } elsif ( defined $self->{structs}->{$typename} ) {
		$metatype = &METATYPE_STRUCT;
		$short_typename = get_idmef_name($typename);

	    } elsif ( defined $self->{enums}->{$typename} ) {
		$metatype = &METATYPE_ENUM;
		$short_typename = get_idmef_name($typename);

	    } elsif ( defined $self->{pre_declareds}->{$typename} ) {
		$metatype = $self->{pre_declareds}->{$typename}->{metatype};
		$short_typename = get_idmef_name($typename);

	    }

	    push(@field_list, { metatype => &METATYPE_NORMAL | $metatype, 
				typename => $typename, 
				short_typename => $short_typename,
				value_type => $value_type,
				name => $name,
				short_name => $name,
				ptr => ($ptr ? 1 : 0),
				dynamic_ident => 0 });

	    $self->debug("parse struct field metatype:normal name:$name typename:$typename ptr:", ($ptr ? 1 : 0) ? "yes" : "no", "\n");
	    
	} elsif ( (($key, $name, $typename) = $line =~ /\s*(KEYLISTED_OBJECT)\(\s*(\w+)\s*,\s*(\w+)\s*\)/) || 
                  (($name, $typename) = $line =~ /\s*LISTED_OBJECT\(\s*(\w+)\s*,\s*(\w+)\s*\)/) ) {
	    my $short_name;
	    my $extra_metatype = 0;
	    my $value_type;

	    $short_name = $name;
	    $short_name =~ s/_list$//;

            if ( defined $key ) {
                 $extra_metatype |= &METATYPE_KEYED_LIST;
            }

	    if ( $self->{primitives}->{$typename} ) {
		$extra_metatype |= &METATYPE_PRIMITIVE;
		$value_type = get_value_type($typename);
	    }

	    push(@field_list, { metatype => &METATYPE_LIST | $extra_metatype,
				typename => $typename,
				short_typename => get_idmef_name($typename),
				name => $name,
				value_type => $value_type,
				short_name => $short_name });
	    $self->debug("parse struct field metatype:list name:$name typename:$typename\n");

	} elsif ( ($typename, $var) = $line =~ /^\s*UNION\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*\{/ ) {
	    my $field = { metatype => &METATYPE_UNION, 
			  typename => $typename,
			  short_typename => get_idmef_name($typename),
			  var => $var,
			  member_list => [ ] };
	    my $member;
	    my($value, $type, $name);

	    $self->debug("parse union...\n");
	    
	    while ( defined($line = $self->get_line) ) {
		push(@{ $struct->{desc} }, $line);
		
		if ( ($value, $typename, $ptr, $name) = $line =~ /^\s*UNION_MEMBER\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\**)(\w+)\s*\);/ ) {
		    $member = { metatype => &METATYPE_NORMAL | &METATYPE_STRUCT,
				value => $value, 
				typename => $typename,
				short_typename => get_idmef_name($typename),
				name => $name,
				ptr => ($ptr ? 1 : 0) };
		    push(@{ $field->{member_list} }, $member);
		    $self->debug("parse union member name:$name typename:$typename value:$value ptr:", ($ptr ? 1 : 0), "\n");

		} elsif ( ($name) = $line =~ /^\s*\}\s*(\w+)\;/ ) {
		    $field->{name} = $name;
		    push(@field_list, $field);
		    $self->debug("parsing of union $name finished\n");
		    last;
		}
	    }

	} elsif ( ($name) = $line =~ /\s*DYNAMIC_IDENT\(\s*(\w+)\s*\)/ ) {
	    push(@field_list, 
		 { metatype => &METATYPE_NORMAL|&METATYPE_PRIMITIVE,
		   typename => "uint64_t",
		   short_typename => "uint64",
		   value_type => "uint64",
		   name => $name,
		   short_name => $name,
		   ptr => 0,
		   dynamic_ident => 1 });
	    $self->debug("parse struct field metatype:normal name:$name dynamic_ident\n");

	} 

	elsif ( ($name) = $line =~ /\s*IS_KEY_LISTED\(\s*(\w+)\s*\)/ ) {
	    $struct->{is_listed} = 1;
	    $struct->{is_key_listed} = 1;
            my $typename = "prelude_string_t";
            my $metatype;
            my $short_typename;
            my $value_type;

            if ( defined $self->{primitives}->{$typename} ) {
                $metatype = $self->{primitives}->{$typename};
                $short_typename = $typename;
                $short_typename =~ s/_t$//;
                $value_type = get_value_type($short_typename);

            } elsif ( defined $self->{structs}->{$typename} ) {
                $metatype = &METATYPE_STRUCT;
                $short_typename = get_idmef_name($typename);
            }
            push(@field_list, { metatype => &METATYPE_NORMAL | $metatype,
                                typename => $typename,
                                short_typename => $short_typename,
                                value_type => $value_type,
                                name => $name,
                                short_name => $name,
                                ptr => 1,
                                required => 0,
                                is_listed => 1,
                                is_key_listed => 1,
                                dynamic_ident => 0 });

        }
        elsif ( ($typename, $name) = $line =~ /\s*OPTIONAL_INT\(\s*(\w+)\s*,\s*(\w+)\s*\)/ ) {
	    my $metatype;
	    my $short_typename;
	    my $value_type;

	    if ( $self->{enums}->{$typename} ) {
		$metatype = &METATYPE_ENUM;
		$short_typename = get_idmef_name($typename);

	    } else {
		$metatype = &METATYPE_PRIMITIVE;
		$short_typename = $typename;
		$short_typename =~ s/_t$//;
	    }

	    $value_type = get_value_type($short_typename);

	    push(@field_list, { metatype => &METATYPE_NORMAL|&METATYPE_OPTIONAL_INT|$metatype,
				typename => $typename,
				short_typename => $short_typename,
				value_type => $value_type,
				name => $name,
				short_name => $name,
				ptr => 0,
				dynamic_ident => 0 });

	} elsif ( ($typename, $id) = $line =~ /^\}\s*TYPE_ID\(\s*(\w+)\s*,\s*(\d+)\s*\)/ ) {
	    $struct->{typename} = $typename;
	    $struct->{short_typename} = get_idmef_name($typename);
	    $struct->{id} = $id;
	    $struct->{field_list} = \@field_list;
	    $self->{structs}->{$typename} = $struct;
	    push(@{ $self->{struct_list} }, $struct);
	    $self->{objs}->{$typename} = $struct;
	    push(@{ $self->{obj_list} }, $struct);
	    $self->debug("parsing of struct $typename (id:$id) finished\n");
	    last;
	}
    }
}

sub	parse_enum
{
    my	$self = shift;
    my	$line = shift;
    my	$enum = { obj_type => &OBJ_ENUM, desc => [ $line ] };
    my	@field_list;
    my	$typename;
    my	$value;
    my	$name;
    my  $text;
    my  $empty;
    my	$id;

    ($enum->{prefix}) = $line =~ /^\s*ENUM\((\w*)\)/;

    while ( defined($line = $self->get_line) ) {
	push(@{ $enum->{desc} }, $line);

	if ( ($name, $empty, $text, $value) = $line =~ /^\s*([^\( ]+)(\(([^\)]+)*\))?\s*\=\s*(\d+)/ ) {
	    push(@field_list, { name => $name, text => $text, value => $value });
	    
	    $text = $text || "";
	    $self->debug("parse enum field name:'$name' text:'$text' value:'$value'\n");
	    
	} elsif ( ($typename, $id) = $line =~ /^\}\s*TYPE_ID\(\s*(\w+)\s*,\s*(\d+)\s*\)/ ) {
	    $enum->{typename} = $typename;
	    $enum->{short_typename} = get_idmef_name($typename);
	    $enum->{id} = $id;
	    $enum->{field_list} = \@field_list;
	    $self->{enums}->{$typename} = $enum;
	    push(@{ $self->{enum_list} }, $enum);
	    $self->{objs}->{$typename} = $enum;
	    push(@{ $self->{obj_list} }, $enum);
	    $self->debug("parsing of enum $typename (id:$id)\n");
	    last;
	}
    }
}

sub	parse_pre_declared
{
    my	$self = shift;
    my	$line = shift;
    my	$pre_declared = { obj_type => &OBJ_PRE_DECLARED };
    my	$typename;
    my	$class;

    ($typename, $class) = $line =~ /^\s*PRE_DECLARE\(\s*(\w+)\s*,\s*(\w+)\s*\)/;

    $pre_declared->{typename} = $typename;
    $pre_declared->{short_typename} = get_idmef_name($typename);

    $pre_declared->{metatype} = &METATYPE_STRUCT if ( $class eq "struct" );
    $pre_declared->{metatype} = &METATYPE_ENUM if ( $class eq "enum" );

    $self->{pre_declareds}->{$typename} = $pre_declared;
    push(@{ $self->{pre_declared_list} }, $pre_declared);

    push(@{ $self->{obj_list} }, $pre_declared);

    $self->debug("parse pre_declared type:$typename class:$class\n");
}

sub	parse_primitive
{
    my	$self = shift;
    my	$line = shift;
    my	$type;

    ($type) = $line =~ /^\s*PRIMITIVE_TYPE\(\s*(\w+)\s*\)/;
    $self->{primitives}->{$type} = &METATYPE_PRIMITIVE;
    $self->debug("parse primitive type:$type\n");
}

sub	parse_primitive_struct
{
    my	$self = shift;
    my	$line = shift;
    my	$type;

    ($type) = $line =~ /^\s*PRIMITIVE_TYPE_STRUCT\(\s*(\w+)\s*\)/;
    $self->{primitives}->{$type} = &METATYPE_PRIMITIVE | &METATYPE_STRUCT;
    $self->debug("parse primitive struct type:$type\n");
}

sub	process
{
    my	$self = shift;
    my	$generator = shift;

    $generator->header($self) if ( $generator->can("header") );

    foreach my $obj ( @{ $self->{obj_list} } ) {
	
	if ( $obj->{obj_type} == &OBJ_STRUCT ) {
	    $generator->struct($self, $obj) if ( $generator->can("struct") );
	}
		
	if ( $obj->{obj_type} == &OBJ_PRE_DECLARED ) {
	    $generator->pre_declared($self, $obj) if ( $generator->can("pre_declared") );
	}
	
	if ( $obj->{obj_type} == &OBJ_ENUM ) {
	    $generator->enum($self, $obj) if ( $generator->can("enum") );
	}
    }

    foreach my $obj ( @{ $self->{obj_list} } ) {
	
	$generator->obj($self, $obj) if ( $generator->can("obj") );

	if ( $obj->{obj_type} == &OBJ_STRUCT ) {
	    $generator->struct_func($self, $obj) if ( $generator->can("struct_func") );
	}
    }

    $generator->footer($self) if ( $generator->can("footer") );
}

1;
