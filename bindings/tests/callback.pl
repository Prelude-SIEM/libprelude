#!/usr/bin/perl -w
#

use strict;

push @INC,".";
push @INC,"./perl";
push @INC,"./.libs";

eval  { require PreludeEasy; };
die "Could not load PreludeEasy ($@).\nTry 'cd ./.libs && ln -s libprelude_perl.so PreludeEasy.so'" if $@;

sub PrintUID
{
	print "UID is $<\n";
}

PreludeEasy::set_perlmethod(\&PrintUID);

PreludeEasy::test_fct();
