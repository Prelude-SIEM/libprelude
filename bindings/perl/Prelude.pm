# Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
# All Rights Reserved
#
# This file is part of the Prelude program.
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

require XSLoader;
XSLoader::load("Prelude");

use strict;

package Prelude;

sub	value2scalar($)
{
    my	$value = shift;
    my	$type;
    my	$result;

    $type = Prelude::idmef_value_get_type($value);

    if ( $type == $Prelude::IDMEF_VALUE_TYPE_INT16 ) {
	$result = Prelude::idmef_value_get_int16($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_UINT16 ) {
	$result = Prelude::idmef_value_get_uint16($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_INT32 ) {
	$result = Prelude::idmef_value_get_int32($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_UINT32) {
	$result = Prelude::idmef_value_get_uint32($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_INT64 ) {
	$result = Prelude::idmef_value_get_int64($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_UINT64) {
	$result = Prelude::idmef_value_get_uint64($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_FLOAT ) {
	$result = Prelude::idmef_value_get_float($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_DOUBLE ) {
	$result = Prelude::idmef_value_get_double($value);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_STRING ) {
	my $string;

	$string = Prelude::idmef_value_get_string($value) or return undef;
	$result = Prelude::idmef_string_get_string($string);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_ENUM) {
	$result = Prelude::idmef_type_enum_to_string(Prelude::idmef_value_get_object_type($value),
						     Prelude::idmef_value_get_enum($value));

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_TIME ) {
	my $time;

	$time = Prelude::idmef_value_get_time($value) or return undef;

	$result = { };
	$result->{sec} = Prelude::idmef_time_get_sec($time);
	$result->{usec} = Prelude::idmef_time_get_usec($time);

    } elsif ( $type == $Prelude::IDMEF_VALUE_TYPE_DATA ) {
	my $data;

	$data = Prelude::idmef_value_get_data($value) or return undef;
	$result = Prelude::idmef_data_get_data($data);

    } else {
	warn "type $type not supported !\n";
    }

    return $result;
}


package Client;

sub RECV_IDMEF { $Prelude::PRELUDE_CLIENT_CAPABILITY_RECV_IDMEF }
sub SEND_IDMEF { $Prelude::PRELUDE_CLIENT_CAPABILITY_SEND_IDMEF }
sub RECV_ADMIN { $Prelude::PRELUDE_CLIENT_CAPABILITY_RECV_ADMIN }
sub SEND_ADMIN { $Prelude::PRELUDE_CLIENT_CAPABILITY_SEND_ADMIN }
sub RECV_CM { $Prelude::PRELUDE_CLIENT_CAPABILITY_RECV_CM }
sub SEND_CM { $Prelude::PRELUDE_CLIENT_CAPABILITY_SEND_CM }

sub new($$)
{
    my $class = shift;
    my $capability = shift || return undef;
    my $name = shift || $0;
    my $config = shift || undef;
    my $self = { };

    $self->{client} = undef;
    $self->{msgbuf} = undef;

    if ( ! $config ) {
	$config = `libprelude-config --prefix`;
	chomp($config);
	$config .= "/etc/prelude/default/client.conf";
    }

    $self->{client} = Prelude::prelude_client_new($Prelude::PRELUDE_CLIENT_CAPABILITY_SEND_IDMEF);
    if ( ! $self->{client} ) {
	return undef;
    }

    if ( Prelude::prelude_client_init($self->{client}, $name, $config, 1, [ $0 ]) < 0 ) {
	Prelude::prelude_client_destroy($self->{client});
	return undef;
    }

    $self->{analyzer} = Prelude::prelude_client_get_analyzer($self->{client});
    if ( ! $self->{analyzer}) {
	Prelude::prelude_client_destroy($self->{client});
	return undef;
    }

    my $process = Prelude::idmef_analyzer_get_process($self->{analyzer});
    
    foreach my $arg ( @ARGV ) {
	Prelude::idmef_process_set_arg($process, Prelude::idmef_string_new_dup($arg));
    }

    if ( $capability & &SEND_IDMEF ) {
	$self->{msgbuf} = Prelude::prelude_msgbuf_new($self->{client});
	if ( ! $self->{msgbuf} ) {
	    Prelude::prelude_client_destroy($self->{client});
	    return undef;
	  }
    }

    return bless($self, $class);
}

sub set_manufacturer
{
    my $self = shift;
    my $manufacturer = shift || return 0;

    Prelude::idmef_analyzer_set_manufacturer($self->{analyzer}, Prelude::idmef_string_new_dup($manufacturer));

    return 1;
}

sub set_model
{
    my $self = shift;
    my $model = shift || return 0;
    my $version = shift || undef;

    Prelude::idmef_analyzer_set_model($self->{analyzer}, Prelude::idmef_string_new_dup($model));

    if ( $version ) {
	Prelude::idmef_analyzer_set_version($self->{analyzer}, Prelude::idmef_string_new_dup($version));
    }

    return 1;
}

sub set_class
{
    my $self = shift;
    my $class = shift || return 0;

    Prelude::idmef_analyzer_set_class($self->{analyzer}, Prelude::idmef_string_new_dup($class));

    return 1;
}

sub send_alert
{
    my $self = shift;
    my $message = shift || return 0;
    my $alert;

    $alert = Prelude::idmef_message_get_alert($$message) || return undef;
    Prelude::idmef_alert_set_analyzer($alert, Prelude::idmef_analyzer_ref($self->{analyzer}));
    Prelude::idmef_write_message($self->{msgbuf}, $$message);
    Prelude::prelude_msgbuf_mark_end($self->{msgbuf});

    return 1;    
}

sub DESTROY
{
    my $self = shift;

    Prelude::prelude_msgbuf_close($self->{msgbuf}) if ( $self->{msgbuf} );
    Prelude::prelude_client_destroy($self->{client}) if ( $self->{client} );
}



package Sensor;

sub new
{
    shift;
    return new Client(&Client::SEND_IDMEF, @_) ;
}



package IDMEFMessage;

sub	new($)
{
    my	$class = shift;
    my	$self;

    $self = Prelude::idmef_message_new();

    return $self ? bless(\$self, $class) : undef;
}

sub	tostring
{
    my	$self = shift;
    my	$buffer = "A" x 8192;
    my	$retval;

    ($retval = Prelude::idmef_message_to_string($$self, $buffer, length $buffer)) < 0 and return undef;

    return substr($buffer, 0, $retval);
}

# FIXME: clean up

sub	set
{
    my	$self = shift;
    my	$object_arg = shift || return 0;
    my	$value_arg = shift || return 0;
    my	$object;
    my	$value;
    my	$ret;

    $object = Prelude::idmef_object_new_fast($object_arg);
    unless ( $object ) {
	return 0;
    }

    if ( Prelude::idmef_object_get_value_type($object) == $Prelude::IDMEF_VALUE_TYPE_TIME ) {
	my $time;

	$time = Prelude::idmef_time_new();
	unless ( $time ) {
	    Prelude::idmef_object_destroy($object);
	    return 0;
	}

	if ( ref $value_arg ) {
	    if ( ref $value_arg eq "ARRAY" ) {
		Prelude::idmef_time_set_sec($time, $value_arg->[0]);
		Prelude::idmef_time_set_usec($time, $value_arg->[1]);

	    } elsif ( ref $value_arg eq "HASH" ) {
		Prelude::idmef_time_set_sec($time, $value_arg->{sec});
		Prelude::idmef_time_set_usec($time, $value_arg->{usec});
	    }

	} else {
	    if ( $value_arg =~ /^\d+$/ ) {
		Prelude::idmef_time_set_sec($time, $value_arg);

	    } else {
		if ( Prelude::idmef_time_set_string($time, $value_arg) < 0 ) {
		    Prelude::idmef_object_destroy($object);
		    Prelude::idmef_time_destroy($time);
		    return 0;
		}
	    }
	}

	$value = Prelude::idmef_value_new_time($time);
	unless ( $value ) {
	    Prelude::idmef_object_destroy($object);
	    Prelude::idmef_time_destroy($time);
	    return 0;
	}
	
    } else {
	$value = Prelude::idmef_value_new_for_object($object, $value_arg);
	unless ( $value) {
	    Prelude::idmef_object_destroy($object);
	    return 0;
	}
    }

    $ret = Prelude::idmef_object_set($$self, $object, $value);

    Prelude::idmef_object_destroy($object);
    Prelude::idmef_value_destroy($value);

    return ($ret == 0);
}

sub	_convert_value
{
    my	$value = shift;
    my	$retval;

    return undef unless ( defined $value );

    return ($value ? Prelude::value2scalar($value) : undef) unless ( Prelude::idmef_value_is_list($value) );

    $retval = [ ];

    for ( 0 .. Prelude::idmef_value_get_count($value) - 1 ) {
	push(@{ $retval }, _convert_value(Prelude::idmef_value_get_nth($value, $_)));
    }

    return $retval;
}

sub	get
{
    my	$self = shift;
    my	@object_str_list = @_;
    my	$object;
    my	$value;
    my	@ret;

    foreach my $object_str ( @object_str_list ) {
	$object = Prelude::idmef_object_new_fast($object_str) or return ();
	$value = Prelude::idmef_object_get($$self, $object);
	Prelude::idmef_object_destroy($object);
	push(@ret, _convert_value($value));
	Prelude::idmef_value_destroy($value) if ( $value );
    }

    return wantarray ? @ret : $ret[0];
}

sub	print
{
    my	$self = shift;

    Prelude::idmef_message_print($$self);
}

sub	DESTROY
{
    my	$self = shift;

    $$self and Prelude::idmef_message_destroy($$self);    
}



package	IDMEFAlert;

sub	new
{
    my	$class = shift;
    my	$self;
    my	@argv;

    $self = new IDMEFMessage() or return undef;

    $self->set("alert.create_time", time());

    return $self;
}



package IDMEFCriteria;

sub	new
{
    my	$class = shift;
    my	$self;

    $self = (defined $_[0] ?
	     Prelude::idmef_criteria_new_string(shift) :
	     Prelude::idmef_criteria_new());

    return $self ? bless(\$self, $class) : undef;
}

sub	clone
{
    my	$self = shift;
    my	$new;

    $new = Prelude::idmef_criteria_clone($$self);

    return $new ? bless(\$new, "IDMEFCriteria") : undef;
}

sub	print
{
    my	$self = shift;

    Prelude::idmef_criteria_print($$self);
}

sub	tostring
{
    my	$self = shift;
    my	$buffer = "A" x 512;
    my	$retval;

    ($retval = Prelude::idmef_criteria_to_string($$self, $buffer, length $buffer)) < 0 and return undef;

    return substr($buffer, 0, $retval);
}

sub	_add
{
    my	$self = shift;
    my	$add_function = shift;
    my	$criteria;

    if ( ref $_[0] eq "IDMEFCriteria" ) {
	my $tmp = $_[0];

	$criteria = Prelude::idmef_criteria_clone($$tmp);

    } else {
	$criteria = Prelude::idmef_criteria_new_string($_[0]);
    }

    return 0 unless ( $criteria );

    &$add_function($$self, $criteria);

    return 1; 
}

sub	and
{
    my	$self = shift;

    return $self->_add(&Prelude::idmef_criteria_and_criteria, @_);
}

sub	or
{
    my	$self = shift;

    return $self->_add(&Prelude::idmef_criteria_or_criteria, @_);
}

sub	DESTROY
{
    my	$self = shift;

    $$self and Prelude::idmef_criteria_destroy($$self);
}

1;


=head1 NAME

Prelude - Perl binding for libprelude

=head1 SYNOPSIS

    use Prelude;

    Prelude::sensor_init("my-sensor");

    my $alert;

    $alert = new IDMEFAlert;

    $alert->set("alert.detect_time" => time);

    $alert->set("alert.assessment.impact.severity" => "medium");
    $alert->set("alert.assessment.impact.completion" => "success");
    $alert->set("alert.assessment.impact.type" => "recon");
    $alert->set("alert.assessment.impact.description" => "An abnormal tcp connection has been detected");
    $alert->set("alert.classification.name" => "Abnormal access");

    $alert->set("alert.source(0).node.address(0).category" => "ipv4-addr");
    $alert->set("alert.source(0).node.address(0).address" => "192.168.0.2");
    $alert->set("alert.source(0).service.port" => 25687);
    $alert->set("alert.source(0).service.protocol" => "tcp");

    $alert->set("alert.target(0).node.address(0).category" => "ipv4-addr");
    $alert->set("alert.target(0).node.address(0).address" => "192.168.0.3");
    $alert->set("alert.target(0).service.port" => 80);
    $alert->set("alert.target(0).service.name" => "www");
    $alert->set("alert.target(0).service.protocol" => "tcp");

    $alert->send;

=head1 DESCRIPTION

Prelude.pm allows you to write prelude sensors in perl.
It also provide classes useful classes needed by PreludeDB.

Prelude.pm provides the following classes:

=over 4

=item B<IDMEFMessage>

=item B<IDMEFAlert>

IDMEF message manipulation classes.

=item B<IDMEFCriteria>

IDMEF criteria building class.

=back

=head1 FUNCTIONS

=over 4

=item B<< Prelude::sensor_init ( $sensor_name ) >>

Initialize the prelude sensor.
If no $sensor_name is given, $0 (the program name) will be used.


=back

=head1 IDMEFMessage class and IDMEFAlert pseudo class

=head2 Constructors

There are two ways to build a new IDMEF message:

=over 4

=item B<$message = new IDMEFMessage>

General purpose constructor: create a new, empty IDMEF message.

=item B<$alert = new IDMEFAlert>

Create a new IDMEF alert message and automatically sets the alert.create_time and
alert.analyzer.* fields.

=back

=head2 Methods

the C<new IDMEFAlert> constructor is simply a wrapper for message creation, thus
all the following methods are available for both IDMEFMessage and IDMEFAlert created objects.


=over 4

=item B<< $message->set(idmef_object => value) >>

Set the value of an idmef_object in the given message.
The function returns 1 if success, 0 otherwise.

Example:
    $message->set("alert.detect_time" => time());

=item B<< $value = $message->get(idmef_object) >>

Return the value of an idmef_object in the message.
The function returns undef if something goes wrong.
If the idmef_object contains one or more elements that are listed
(like alert.classification().name or alert.source().node.address().address)
you can retrieve the value of a specific element of the list, for example
$value = $message->get("alert.classification(0).name") or the whole list:
$value = $message->get("alert.classification.name"), in this case, $value
is a reference to an array of all alert.classification.name values

Examples:
    $time = $message->get("alert.detect_time");

    $value = $message->get("alert.classification(0).name");

    $value = $message->get("alert.classification.name");

    foreach ( @{ $value } ) {
	# do something with $_
    }

    $value = $message->get("alert.source.node.address.address");

    foreach $source ( @{ $value } ) {
	foreach $address ( @{ $source } ) {
	    # do something with address
	}
    }

=item B<< $message->print >>

Dump the content of the message on STDOUT.

=item B<< $dump = $message->tostring >>

Dump the content of the message in the returned scalar.

=item B<< $message->send >>

Send the message to the manager.

=back

=head1 IDMEFCriteria class

The IDMEFCriteria class plays with criteria strings.
A criteria string describe the relation between objects and values.

Example:
    C<< alert.detect_time >= 2003-01-01 >>

The relations between an object and a value can be:
    C<< == >>, C<< != >>, C<< >= >>, C<< <= >>, C<< =~ >>, C<< substr >>.

A criteria can be built up of several criterion joined with C<< && >>
or C<< || >>.

Example:
   C<< alert.detect_time >= 2003-01-01 && alert.detect_time < 2004-01-01 >>

Sub criteria can also be created with C<< ( ) >>.

Example:
   C<< alert.detect_time >= 2003-01-01 && (alert.target.service.port == 80 || alert.target.service.port == 8000) >>

=head2 Constructor

=over 4

=item B< new IDMEFCriteria(criteria_string) >

Create a new IDMEFCriteria object from criteria_string.
If no criteria_string argument is given, an empty IDMEFCriteria object
is built.

Example:
    $criteria = new IDMEFCriteria("idmef.target.service.port == 80");

=item B<< $new_criteria = $criteria->clone >>

Create a clone/copy of a given criteria.

=item B<< $criteria->print >>

Dump the criteria on STDOUT.

=item B<< $criteria->tostring >>

Dump the criteria in the returned scalar.

=item B<< $criteria->and(criteria_new) >>

Append a new criteria to a given criteria with operator C<< && >>, criteria_new can be either
a criteria string or a criteria object.

Example:
    $criteria = new IDMEFCriteria();
    $criteria->and("alert.create_time < 2003-11-12");
    $criteria->and(new IDMEFCriteria("alert.target.service.port == 80"));

=item B<< $criteria->or(criteria_new) >>

Same as above with operator C<< || >>.

=back

=head1 AUTHOR

=over4

Nicolas Delon <delon.nicolas@wanadoo.fr>

=back

=cut
