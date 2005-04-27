#!/usr/bin/perl -w
#
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

use strict;

use IDMEFTree;
use GenerateDebug;
use GenerateIDMEFTreeWrapC;
use GenerateIDMEFTreeWrapH;
use GenerateIDMEFTreeData;
use GenerateIDMEFMessageWriteC;
use GenerateIDMEFMessageWriteH;
use GenerateIDMEFMessageReadC;
use GenerateIDMEFMessageReadH;
use GenerateIDMEFMessageIdH;
use GenerateIDMEFMessagePrintC;
use GenerateIDMEFMessagePrintH;
use GenerateIDMEFValueClassSwigMapping;

sub	target_need_update
{
    my	$generator = shift;
    my	@file_list = @_;
    my	$source = $generator->{source};
    my	$target = $generator->{target};
    my	$source_time = (stat $source)[9];
    my	$target_time = (stat $target)[9];

    foreach my $file ( @file_list ) {
	return 1 if ( (stat $file)[9] > $target_time );
    }

    return ($source_time > $target_time);
}

sub	target_update
{
    my	$generator = shift;
    my	$idmef_tree = shift;
    my	$target = $generator->{target};
    my	$func = $generator->{func};

    $generator = &$func($target);
    $idmef_tree->process($generator);
}

my $idmef_tree;
my $generator;
my @generator_list = ({ source => 'GenerateIDMEFTreeWrapC.pm', 
			target => '../idmef-tree-wrap.c',
			func => sub { new GenerateIDMEFTreeWrapC(-filename => shift) } },

		      { source => 'GenerateIDMEFTreeWrapH.pm',
			target => '../include/idmef-tree-wrap.h',
			func => sub { new GenerateIDMEFTreeWrapH(-filename => shift) } },

		      { source => 'GenerateIDMEFTreeData.pm',
			target => '../include/idmef-tree-data.h',
			func => sub { new GenerateIDMEFTreeData(-filename => shift) } },

		      { source => 'GenerateIDMEFMessagePrintC.pm',
			target => '../idmef-message-print.c',
			func => sub { new GenerateIDMEFMessagePrintC(-filename => shift) } },

		      { source => 'GenerateIDMEFMessagePrintH.pm',
			target => '../include/idmef-message-print.h',
			func => sub { new GenerateIDMEFMessagePrintH(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageWriteC.pm',
			target => '../idmef-message-write.c',
			func => sub { new GenerateIDMEFMessageWriteC(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageWriteH.pm',
			target => '../include/idmef-message-write.h',
			func => sub { new GenerateIDMEFMessageWriteH(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageReadC.pm',
			target => '../idmef-message-read.c',
			func => sub { new GenerateIDMEFMessageReadC(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageReadH.pm',
			target => '../include/idmef-message-read.h',
			func => sub { new GenerateIDMEFMessageReadH(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageIdH.pm',
			target => '../include/idmef-message-id.h',
			func => sub { new GenerateIDMEFMessageIdH(-filename => shift) } },

		      { source => 'GenerateIDMEFValueClasstSwigMapping.pm',
			target => '../../bindings/idmef-value-class-mapping.h',
			func => sub { new GenerateIDMEFValueClassSwigMapping(-filename => shift) } }
		      );

$idmef_tree = new IDMEFTree(-filename => "idmef-tree.h",
			    -debug => 0);
$idmef_tree->load();

foreach my $generator (@generator_list) {
    target_need_update($generator, 'idmef-tree.h', 'IDMEFTree.pm') and target_update($generator, $idmef_tree);
}
