#!/usr/bin/perl -wT

use strict;
use warnings;

use CGI;
use CGI::Carp qw ( fatalsToBrowser );
use File::Basename;

use Digest::MD5 qw(md5 md5_hex md5_base64);
use Time::HiRes qw(gettimeofday); 

use DBI;

my $dbh = DBI->connect('dbi:Pg:dbname=josh', '', '', {RaiseError => 1, PrintError=>0, AutoCommit => 0});

$CGI::POST_MAX = 1024 * 25000;

my $base_dir = "/home/josh/www/predict";
my $bin_dir = "$base_dir/bin";
my $upload_dir = "$base_dir/upload";
my $data_dir = "$base_dir/data";
my $parser_path = "$bin_dir/parser";

my $query = new CGI;
my $filename = $query->param("csv_file");


if ( !$filename )
{
	print $query->header ( );
	print "There was a problem uploading your file.";
	exit;
}

my ($s, $ms) = gettimeofday; 

sub gen_filename
{
	$filename = substr md5_base64 ("i2pi" . rand()*(1<<48) . $s . "2pisalt" . $ms ), -8;
	$filename =~ s/\+/A/g;
	$filename =~ s/\-/l/g;
	$filename =~ s/\//q/g;
}

my $file_path;

do
{
	gen_filename;
	$file_path = "$upload_dir/$filename";
} while (-e $file_path);

my $upload_filehandle = $query->upload("csv_file");

open ( UPLOADFILE, ">$file_path" ) or die "$!";
binmode UPLOADFILE;

while ( <$upload_filehandle> )
{
	my $str = $_;
 print UPLOADFILE;
}

close UPLOADFILE;

my $status = 0;

{
	local $ENV{"PATH"} = "/bin:/usr/local/bin:/usr/bin:$bin_dir"; 
	system ($parser_path, $file_path);
	$status = $? >> 8;
	system ("mv", "$file_path.json", "$data_dir");
}

$dbh->do("INSERT INTO predict.file (file_id, added, parse_status) VALUES (?, now(), ?)", undef, $filename, $status);
$dbh->commit();

print $query->redirect( -URL => $filename);

print $filename;

exit;
