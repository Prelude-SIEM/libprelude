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

package Generate;

use strict;

sub     new
{
    my  $class = shift;
    my  %opt = @_;
    my  $self = { };

    $self->{filename} = $opt{-filename};
    open(FILE, ">$self->{filename}") or die "$self->{filename}: $!";
    $self->{file} = \*FILE;
    bless($self, $class);

    return $self;
}

sub     output
{
    my  $self = shift;
    my  $file = $self->{file};

    print $file @_;
}

1;
