#!/usr/bin/env perl
use strict;
use warnings;
use Furl;
use JSON;
use Archive::Zip qw/:ERROR_CODES :CONSTANTS/;
use FindBin;
use File::chdir;
use Path::Class;
use Config::Pit;

my $config = pit_get('irkit/device/scripts/release.pl', require => {
    access_token => 'github access token'
});

my $user_name = 'irkit';
my $repo_name = 'device';

my $build_script           = "$FindBin::Bin/../firmware/build.sh";
my $hexfile                = "$FindBin::Bin/../firmware/.build/irkit/firmware.hex";
my $zipfile                = "$FindBin::Bin/../firmware/.build/irkit/firmware.zip";
my $tags_endpoint          = sub { return "https://api.github.com/repos/$user_name/$repo_name/tags"; };
my $releases_endpoint      = sub { return "https://api.github.com/repos/$user_name/$repo_name/releases"; };
my $upload_assets_endpoint = sub {
    my $id = shift;
    return "https://uploads.github.com/repos/$user_name/$repo_name/releases/$id/assets?name=firmware.zip";
};
my $client = Furl->new( agent => 'IRKitReleaser/1.0' );

main();

sub main {
    print "fetching newest tag name\n";
    my $newest_tag_name = fetch_newest_tag_name();
    print "newest tag name is $newest_tag_name\n";

    print "fetching newest release name\n";
    my $newest_release_name = fetch_newest_release_name();
    if ($newest_release_name) {
        print "newest release name is $newest_release_name\n";
    }
    else {
        print "no releases\n";
    }

    if ($newest_release_name && ($newest_tag_name eq $newest_release_name)) {
        die "newest tag == newest release\n".
            "run `git push --tags origin master` before creating a release\n";
    }

    print "release tag: $newest_tag_name ? Ctrl+C to abort\n";
    getc();

    print "building firmware.hex\n";
    build();

    print "zipping firmware.hex -> firmware.zip\n";
    zip( $hexfile => $zipfile );

    print "creating a release from $newest_tag_name\n";
    my $release_info = create_release( $newest_tag_name );
    print "created release id: $release_info->{ id }\n";
    
    print "uploading firmware.zip\n";
    upload_asset( $release_info->{ id } => $zipfile );
    print "successfully uploaded firmware.zip\n";

    my $command = "open \"https://github.com/$user_name/$repo_name/releases\"";
    qx{ $command };
}

sub fetch_newest_tag_name {
    my $res = $client->get( $tags_endpoint->() );
    die $res->status_line unless $res->is_success;

    my $tags = decode_json( $res->content );
    return $tags->[ 0 ]{ name };
}

sub fetch_newest_release_name {
    my $res = $client->get( $releases_endpoint->() );
    die $res->status_line unless $res->is_success;

    my $releases = decode_json( $res->content );
    return $releases->[ 0 ]{ name };
}

sub build {
    local $CWD = "$FindBin::Bin/../firmware";

    my $command = "$build_script";
    my $stdout  = qx{ $command };
    if ($? != 0) {
        die "$command failed with exit code: $?";
    }
    print $stdout;
}

sub zip {
    my ($infile, $outfile) = @_;

    my $zip = Archive::Zip->new;
    $zip->addFile( $infile, file($infile)->basename )
        or die "failed to add $infile";

    unless ($zip->writeToFileNamed( $outfile ) == AZ_OK) {
        die "zip write failed";
    }
    return $outfile;
}

sub create_release {
    my $tag_name = shift;

    my $body = "https://github.com/$user_name/$repo_name/commits/$tag_name";

    my $res = $client->post( $releases_endpoint->(), [
        'Content-Type'  => 'application/x-www-form-urlencoded',
        'Authorization' => "token $config->{ access_token }",
    ], encode_json({
        tag_name => $tag_name,
        name     => $tag_name,
        body     => "see $body",
    }) );
    die $res->status_line unless $res->is_success;

    return decode_json( $res->content );
}

sub upload_asset {
    my ($release_id, $zipfile_path) = @_;

    my $res = $client->post( $upload_assets_endpoint->( $release_id ), [
        'Content-Type'  => 'application/zip',
        'Authorization' => "token $config->{ access_token }",
    ], file($zipfile_path)->openr );
    die $res->status_line unless $res->is_success;

    return decode_json( $res->content );
}
