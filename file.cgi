#!/usr/bin/perl -wT

use strict;
use warnings;

use CGI;
use CGI::Cookie;
use CGI::Carp qw ( fatalsToBrowser );
use File::Basename;

use Digest::MD5 qw(md5 md5_hex md5_base64);
use Time::HiRes qw(gettimeofday); 

use DBI;

my $dbh = DBI->connect('dbi:Pg:dbname=josh', '', '', {RaiseError => 1, PrintError=>0, AutoCommit => 0});



$CGI::POST_MAX = 1024 * 5000;

my $base_dir = "/home/josh/www/predict";
my $data_dir = "$base_dir/data";

my $html_path = "$base_dir/file.xhtml";

my $query = new CGI;
my $filename = $query->param("file_name");

my $cookie = $query->cookie("uid");
if (!defined $cookie)
{
	my $uid = substr md5_base64 ("hello" . rand()*(1<<48) . "2isalt" ), 10;
    $uid =~ s/\+/_/g;
    $uid =~ s/\//-/g;

	$cookie = $query->cookie(-name=>'uid',
		-value=>$uid,
		-expires=>'+12M',
		-domain=>'predict.i2pi.com');	
}




if ( !$filename )
{
	print $query->header (-cookie=>$cookie);
	print "No such file";
	exit;
}

my $json_path = "$data_dir/$filename.json";

if ( ! -f $json_path )
{
	print $query->header (-cookie=>$cookie);
	print <<END_HTML;
	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "DTD/xhtml1-strict.dtd">
	<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
	 <head>
	   <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	   <title>predict.i2pi Claim $filename</title>
	 </head>
	 <body>
		<h1> Ooops </h1>
		<p>One of two things may have happened to cause this error:
			<ol>
				<li>The system is slow and your file hasn't been processed yet. Give it a moment.</li>
				<li>Your file was in some way invalid and we haven't been able to parse it.</li>
				<li>You are generally browsing around randomly and the 404 has struck.</li>	
			</ol>
		</p>
	 </body>
	</html>
END_HTML
} else
{
	open HTML, '<', $html_path || die "Unable to open meta file. This is not good";
	print "Set-cookie: $cookie\n";
	print "Content-type: application/xhtml+xml\n\n";
	while (<HTML>)
	{
		my $str = $_ ;
		$str =~ s/#FILENAME#/$filename/g;
		print $str;
	}	
}
