#!/usr/bin/env perl

use PreludeEasy;

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


PreludeEasy::PreludeLog::SetCallback(\&log_func);

$idmef = new PreludeEasy::IDMEF;

print "*** IDMEF->Set() ***\n";
$idmef->Set("alert.classification.text", "My Message");
$idmef->Set("alert.source(0).node.address(0).address", "s0a0");
$idmef->Set("alert.source(0).node.address(1).address", "s0a1");
$idmef->Set("alert.source(1).node.address(0).address", "s1a0");
$idmef->Set("alert.source(1).node.address(1).address", "s1a1");
$idmef->Set("alert.source(1).node.address(2).address", undef);
$idmef->Set("alert.source(1).node.address(3).address", "s1a3");
print $idmef->ToString();


print "\n*** IDMEF->Get() ***\n";
print $idmef->Get("alert.classification.text") . "\n";

print "\n\n*** Value IDMEF->Get() ***\n";
print $idmef->Get("alert.classification.text");

print "\n\n*** Listed Value IDMEF->Get() ***\n";
print_array($idmef->Get("alert.source(*).node.address(*).address"));

print "\n\n*** Object IDMEF->Get() ***\n";
print $idmef->Get("alert.source(0).node.address(0)");

print "\n\n*** Listed Object IDMEF->Get() ***\n";
print_array($idmef->Get("alert.source(*).node.address(*)"));


open FILE, ">foo.bin" or die "arg";
$idmef->Write(FILE);
close FILE;

print "\n\n*** IDMEF->Read() ***\n";
open FILE2, "<foo.bin" or die "arg2";
my $idmef2 = new PreludeEasy::IDMEF;
while ( $idmef2->Read(FILE2) ) {
	print $idmef2->ToString();
}

close FILE2;

print "\n*** Client ***";
$client = new PreludeEasy::ClientEasy("prelude-lml");
$client->Start();

$client->SendIDMEF($idmef);
