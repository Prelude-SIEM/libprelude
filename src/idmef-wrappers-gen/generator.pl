#!/usr/bin/perl -w
#
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

use strict;

use IDMEFTree;
use GenerateDebug;
use GenerateIDMEFTreeWrapC;
use GenerateIDMEFTreeWrapH;
use GenerateIDMEFTreeData;
use GenerateIDMEFMessageSendC;
use GenerateIDMEFMessageSendH;
use GenerateIDMEFMessageRecvC;
use GenerateIDMEFMessageRecvH;
use GenerateIDMEFMessageIdH;
use GenerateIDMEFTreePrintC;
use GenerateIDMEFTreePrintH;
use GenerateIDMEFTreeToStringC;
use GenerateIDMEFTreeToStringH;

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

		      { source => 'GenerateIDMEFTreePrintC.pm',
			target => '../idmef-tree-print.c',
			func => sub { new GenerateIDMEFTreePrintC(-filename => shift) } },

		      { source => 'GenerateIDMEFTreePrintH.pm',
			target => '../include/idmef-tree-print.h',
			func => sub { new GenerateIDMEFTreePrintH(-filename => shift) } },

		      { source => 'GenerateIDMEFTreeToStringC.pm',
			target => '../idmef-tree-to-string.c',
			func => sub { new GenerateIDMEFTreeToStringC(-filename => shift) } },

		      { source => 'GenerateIDMEFTreeToStringH.pm',
			target => '../include/idmef-tree-to-string.h',
			func => sub { new GenerateIDMEFTreeToStringH(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageSendC.pm',
			target => '../idmef-message-send.c',
			func => sub { new GenerateIDMEFMessageSendC(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageSendH.pm',
			target => '../include/idmef-message-send.h',
			func => sub { new GenerateIDMEFMessageSendH(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageRecvC.pm',
			target => '../idmef-message-recv.c',
			func => sub { new GenerateIDMEFMessageRecvC(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageRecvH.pm',
			target => '../include/idmef-message-recv.h',
			func => sub { new GenerateIDMEFMessageRecvH(-filename => shift) } },

		      { source => 'GenerateIDMEFMessageIdH.pm',
			target => '../include/idmef-message-id.h',
			func => sub { new GenerateIDMEFMessageIdH(-filename => shift) } }
		      );

$idmef_tree = new IDMEFTree(-filename => "../include/idmef-tree.h",
			    -debug => 0);
$idmef_tree->load();

foreach my $generator (@generator_list) {
    target_need_update($generator, '../include/idmef-tree.h', 'IDMEFTree.pm') and target_update($generator, $idmef_tree);
}
