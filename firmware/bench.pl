#!/bin/env perl
use strict;
use warnings;
use Furl;
use Time::HiRes qw/gettimeofday tv_interval/;

my $target_ip = shift;
if (! $target_ip || ($target_ip !~ m![0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}!)) {
    die "usage: $0 {IRKit ip address, ex:10.0.1.9} {number-of-requests}";
}
my $base = "http://${target_ip}";

my $number_of_requests = shift;
if (! $number_of_requests) {
    die "usage: $0 {IRKit ip address, ex:10.0.1.9} {number-of-requests}";
}

my $agent = Furl->new(
    agent   => 'Bench/1.0',
    timeout => 20,
);

my $post_message = sub {
    # エアコンオン
    print "POST /messages\n";
    return $agent->post( "$base/messages", [],
                            'message={"freq":38,"format":"raw","data":[6573,3257,867,824,866,825,865,2463,866,826,864,2462,867,825,864,827,863,828,863,2464,866,2462,867,826,864,826,865,826,864,2463,866,2461,868,825,865,826,864,827,864,826,865,826,864,826,865,826,864,826,865,825,866,826,864,826,865,826,864,827,864,2462,867,826,864,826,865,826,864,827,864,827,864,826,864,826,865,2461,868,825,865,826,865,826,864,827,864,2462,867,2461,867,2461,868,2461,867,2461,868,2461,867,2461,868,2461,867,826,864,826,865,2461,866,826,864,827,864,828,863,827,864,826,865,826,865,825,865,826,865,2462,867,2462,866,825,865,826,865,2462,867,825,865,826,865,825,865,826,865,2461,868,825,865,2462,867,2462,867,825,865,826,864,827,864,825,865,826,865,826,865,825,865,826,865,826,864,827,864,826,865,826,864,826,865,2462,867,825,865,827,864,825,864,827,863,827,863,829,863,827,864,827,864,826,865,826,864,826,865,826,865,825,865,827,864,827,864,826,864,827,864,826,864,826,865,826,865,826,864,826,865,826,864,827,864,827,864,826,864,827,864,826,865,2462,867,825,865,2462,867,825,865,826,864,826,865,2463,866,2461,867,826,865,825,865,827,864,2462,867,2461,867]}',
                        );
};

my $post_keys = sub {
    print "POST /keys\n";
    return $agent->post( "$base/keys", [], [] );
};

my @requests = (
    $post_message,
    # $post_keys
);

for my $i (1..$number_of_requests) {
    print "[$i] ";
    my $time = [gettimeofday];
    my $res  = $requests[ int(rand scalar @requests) ]();
    die $res->status_line unless $res->is_success;
    printf( " %d %s\n", $res->code, $res->body );

    # $requests[ 2 ]();
    my $elapsed = tv_interval( $time, [gettimeofday] );
    printf( " %.2f[s]\n", $elapsed );
}
