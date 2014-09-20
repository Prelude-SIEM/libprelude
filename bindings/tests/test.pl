#!/usr/bin/env perl

use Prelude;

sub log_func {
	($level, $str) = @_;
	print("log: " . $str);
}

sub print_array($)
{
    my $arrRef = shift;
    for (my $i = 0; $i < scalar(@$arrRef); ++$i) {
        if (ref($arrRef->[$i]) eq 'ARRAY') {
            print ', ' if ($i);
            print '[';
            print_array($arrRef->[$i]);
            print ']';
        } else {
            print ', ' if ($i);
            print $arrRef->[$i];
        }
    }
    print ' ' if (!scalar(@$arrRef));
}


Prelude::PreludeLog::setCallback(\&log_func);

$idmef = new Prelude::IDMEF;

print "*** IDMEF->Set() ***\n";
$idmef->set("alert.classification.text", "My Message");
$idmef->set("alert.source(0).node.address(0).address", "s0a0");
$idmef->set("alert.source(0).node.address(1).address", "s0a1");
$idmef->set("alert.source(1).node.address(0).address", "s1a0");
$idmef->set("alert.source(1).node.address(1).address", "s1a1");
$idmef->set("alert.source(1).node.address(2).address", undef);
$idmef->set("alert.source(1).node.address(3).address", "s1a3");
print $idmef->toString();


print "\n*** IDMEF->Get() ***\n";
print $idmef->get("alert.classification.text") . "\n";

print "\n\n*** Value IDMEF->Get() ***\n";
print $idmef->get("alert.classification.text");

print "\n\n*** Listed Value IDMEF->Get() ***\n";
print_array($idmef->get("alert.source(*).node.address(*).address"));

print "\n\n*** Object IDMEF->Get() ***\n";
print $idmef->get("alert.source(0).node.address(0)");

print "\n\n*** Listed Object IDMEF->Get() ***\n";
print_array($idmef->get("alert.source(*).node.address(*)"));


open FILE, ">foo.bin" or die "arg";
$idmef->write(FILE);
close FILE;

print "\n\n*** IDMEF->Read() ***\n";
open FILE2, "<foo.bin" or die "arg2";
my $idmef2 = new Prelude::IDMEF;
while ( $idmef2->read(FILE2) ) {
	print $idmef2->toString();
}

close FILE2;

print "\n*** Client ***";
$client = new Prelude::ClientEasy("prelude-lml");
$client->start();

$client->sendIDMEF($idmef);
