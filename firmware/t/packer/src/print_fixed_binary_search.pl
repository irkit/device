#!/usr/bin/env perl
use strict;
use warnings;
use Text::Xslate;
use Data::Section::Simple qw/get_data_section/;

my ($first_value, $multiplier, $reverse) = @ARGV;
die "usage: $0 first_value multiplier"
    if (! $first_value || ! $multiplier);

my @numbers        = make_factorial($first_value, $multiplier);
my @mapped_numbers = ($first_value .. ($first_value + scalar @numbers - 1));
die "numbers and mapped_numbers should have the exact same size"
    if (scalar @numbers != scalar @mapped_numbers);

warn "numbers\t@numbers";
warn "mapped_numbers\t@mapped_numbers";

my $xslate = Text::Xslate->new;
if (! $reverse) {
    print make_binary_tree( \@numbers, \@mapped_numbers, 1 );
}
else {
    print make_binary_tree( \@mapped_numbers, \@numbers, 1 );
}

sub make_factorial {
    my ($first_value, $multiplier) = @_;
    my @ret;
    my $count = 0;
    push( @ret, $first_value );
    while (1) {
        my $next = $ret[ -1 ] * $multiplier;
        last if $next > 65535;
        push( @ret, $next );
    }
    return map { int($_) } @ret;
}

# indent_level starts from 0
sub make_binary_tree {
    my ($numbers, $mapped_numbers, $indent_level) = @_;

    if (scalar @$numbers == 2) {
        my $template = get_data_section('template2.tx');
        return $xslate->render_string( $template, {
            val1   => $numbers->[ 1 ],
            val2   => $mapped_numbers->[ 0 ],
            val3   => $mapped_numbers->[ 1 ],
            indent => '    ' x $indent_level,
        });
    }
    elsif (scalar @$numbers == 1) {
        my $template = get_data_section('template1.tx');
        return $xslate->render_string( $template, {
            val1   => $mapped_numbers->[ 0 ],
            indent => '    ' x $indent_level,
        });
    }
    else {
        my $middle_index = int((scalar @$numbers)/2);
        my $val2         = $numbers->[ $middle_index ];
        my @car          = splice( @$numbers, 0, $middle_index );
        my @mapped_car   = splice( @$mapped_numbers, 0, $middle_index );

        my $template = get_data_section('template_if.tx');
        my $next1 = make_binary_tree(\@car, \@mapped_car, $indent_level+1);
        my $next2 = make_binary_tree($numbers, $mapped_numbers, $indent_level+1);
        return $xslate->render_string( $template, {
            val2   => $val2,
            next1  => $next1,
            next2  => $next2,
            indent => '    ' x $indent_level,
        });
    }
}

__DATA__

@@ template1.tx
<: $indent :>ret = <: $val1 :>;
@@ template2.tx
<: $indent :>if (value < <: $val1 :>) {
<: $indent :>    ret = <: $val2 :>;
<: $indent :>}
<: $indent :>else {
<: $indent :>    ret = <: $val3 :>;
<: $indent :>}
@@ template_if.tx
<: $indent :>if (value < <: $val2 :>) {
<: $next1 | mark_raw :>
<: $indent :>}
<: $indent :>else {
<: $next2 | mark_raw :>
<: $indent :>}
