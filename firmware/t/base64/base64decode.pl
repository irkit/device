#!/usr/bin/env perl
use strict;
use warnings;
use MIME::Base64;

my $encoded = shift;
die "usage: perl $0 {base64 encoded string}\nor\nperl $0 {base64 encoded string} | ../packer/unpack_sequence.bin\n" unless $encoded;

my $decoded = decode_base64($encoded);
print join("", map { sprintf("%02x", ord($_)); } split(//, $decoded));

