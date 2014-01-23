#!/usr/bin/env perl
use strict;
use warnings;
use MIME::Base64;

my $binary = "\x2f\xff\x00\xb2\x2f";
my $encoded = encode_base64($binary);
warn $encoded;
