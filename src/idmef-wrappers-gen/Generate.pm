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
