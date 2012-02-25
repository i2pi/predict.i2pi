#!/usr/bin/perl -wT

use strict;
use warnings;

use File::Basename;

use Digest::MD5 qw(md5 md5_hex md5_base64);
use Time::HiRes qw(gettimeofday); 

use DBI;

my $dbh = DBI->connect('dbi:Pg:dbname=josh', '', '', {RaiseError => 1, PrintError=>0, AutoCommit => 0});

my $base_dir = "/home/josh/www/predict";
my $bin_dir = "$base_dir/bin";
my $upload_dir = "$base_dir/upload";
my $data_dir = "$base_dir/data";
my $parser_path = "$bin_dir/parser";


my ($s, $ms) = gettimeofday; 

my $filename;

sub gen_filename
{
	$filename = substr md5_base64 ("i2pi" . rand()*(1<<48) . $s . "2pisalt" . $ms ), -8;
	$filename =~ s/\+/_/g;
	$filename =~ s/\//-/g;
}

my $file_path;

do
{
	gen_filename;
	$file_path = "$upload_dir/$filename";
} while (-e $file_path);

open ( UPLOADFILE, ">$file_path" ) or die "$!";
binmode UPLOADFILE;
while ( <> )
{
 	print UPLOADFILE $_;
}
close UPLOADFILE;

my $status = 0;

{
	local $ENV{"PATH"} = "/bin:/usr/local/bin:/usr/bin:$bin_dir"; 
	system ($parser_path, $file_path);
	system ("mv", "$file_path.json", "$data_dir");
	$status = $? >> 8;
}

$dbh->do("INSERT INTO predict.file (file_id, added) VALUES (?, now())", undef, $filename);
$dbh->commit();

print "LOADED $filename\n";


exit;
