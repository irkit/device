#!/usr/bin/env perl
use strict;
use warnings;
use MIME::Base64;

my $binary = "\x8e\x76\x01";
my $encoded = encode_base64($binary);
warn $encoded;
