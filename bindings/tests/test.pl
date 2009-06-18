#!/usr/bin/env perl

use PreludeEasy;

sub log_func {
	($level, $str) = @_;
	print("log: " . $str);
}

PreludeEasy::PreludeLog::SetCallback(\&log_func);

$idmef = new PreludeEasy::IDMEF;
$client = new PreludeEasy::ClientEasy("abc");
$client->Start();

print "*** IDMEF->Set() ***\n";
$idmef->Set("alert.classification.text", "My Message");
$idmef->Set("alert.source(0).node.address(0).address", "x.x.x.x");
$idmef->Set("alert.source(0).node.address(1).address", "y.y.y.y");
$idmef->Set("alert.target(0).node.address(0).address", "z.z.z.z");
print $idmef->ToString();


print "\n*** IDMEF->Get() ***\n";
print $idmef->Get("alert.classification.text") . "\n";

sub print_list(@) {
        foreach(@_) {
                if ( ref $_ eq 'ARRAY' ) {
                        print_list(@$_);
                } else {
                        print $_ . "\n";
                }
        }
}

$l = $idmef->Get("alert.source(*).node.address(*).address");
print_list(@$l);

open FILE, ">foo.bin" or die "arg";
$idmef->Write(FILE);
close FILE;

open FILE2, "<foo.bin" or die "arg2";
my $idmef2 = new PreludeEasy::IDMEF;
$idmef2->Read(FILE2);
close FILE2;
print $idmef2->ToString();

print "\n*** Client ***";
$client = new PreludeEasy::ClientEasy("prelude-lml");
$client->Start();

$client->SendIDMEF($idmef);
