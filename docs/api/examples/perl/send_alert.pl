#!/usr/bin/perl -w

use strict;
use Prelude;

my $sensor;
my $alert;

$sensor = Sensor("test");

$alert = new IDMEFAlert();

$alert->set("alert.classification(0).name" => "test perl");
$alert->set("alert.assessment.impact.severity" => "low");
$alert->set("alert.assessment.impact.completion" => "failed");
$alert->set("alert.assessment.impact.type" => "recon");
$alert->set("alert.detect_time" => time());
$alert->set("alert.source(0).node.address(0).address" => "10.0.0.1");
$alert->set("alert.target(0).node.address(0).address" => "10.0.0.2");
$alert->set("alert.target(1).node.address(0).address" => "10.0.0.3");
$alert->set("alert.additional_data(0).data" => "something");

$sensor->send_alert($alert);
