#!/usr/bin/perl -wT

use strict;
use warnings;

use CGI;
#use CGI::Carp qw ( fatalsToBrowser );
use JSON;

use DBI;

my $dbh = DBI->connect('dbi:Pg:dbname=josh', '', '', {RaiseError => 1, PrintError=>1, AutoCommit => 0});


use Data::Dumper;

my $base_dir = "/home/josh/www/predict";
my $data_dir = "$base_dir/data";


my $query = new CGI;
my $json = $query->param("csv");
my $fileID = $query->param("fileID");

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


print $query->header (-cookie=>$cookie);

if (!defined $fileID)
{
	print "ERROR: Must supply 'fileID'\n";
	exit;
}

my $safe_file_id;
if ($fileID =~ /([A-Za-z0-9_\-=]{8})/)
{
	$safe_file_id = $1;	
} else
{
	print "ERROR: Invalid fileID\n";
	exit;
}

if ($fileID eq 'zK7tuI9Q')
{
	print "ERROR: Sorry, can't edit the demo data";
	exit;
}

my $data_path = "$data_dir/$safe_file_id.json";
 
my $orig_json;
{
	local( $/, *FH ) ;
	open( FH, $data_path ) or die ("invalid file id");
	$orig_json= <FH>;
}

my $orig_csv = jsonToObj ($orig_json);
my %orig_hash = %$orig_csv;



my $x = $orig_hash{'column'};
my $ncol = scalar(@$x);

if (!defined $json)
{
	print "ERROR: Must supply 'csv' parameter\n";
	exit;	
}

my $csv = jsonToObj ($json);

my %hash = %$csv;
$x = $hash{'column'};
my @cols = @$x;

# login, if needed

sub error {
	print "{\"error\": {\"message\":\"invalid username or password\", \"context\":\"login\"}}";
	exit;
}


if (defined $hash{'password'})
{
	my $salt = (rand() * (1<<8)) . "i2psalt";	
	my ($username) = $dbh->selectrow_array("SELECT username FROM predict.account WHERE username = ? AND password = md5(salt||?)", undef, 
			$hash{'username'}, $hash{'password'});

	if (!defined $username)
	{
		# Create user if none exists
		$dbh->do("INSERT INTO predict.account (username, salt, password, added) VALUES (?,?, md5(? || ?), now())", undef, $hash{'username'}, $salt, $salt, $hash{'password'});
		$dbh->commit();
	}

	($username) = $dbh->selectrow_array("SELECT username FROM predict.account WHERE username = ? AND password = md5(salt||?)", undef, 
			$hash{'username'}, $hash{'password'});
	error() if (!defined $username);

	$dbh->do("INSERT INTO predict.claim (username, file, added) VALUES (?, ?, now())", undef, $username, $safe_file_id);
	$dbh->commit();

	($username) = $dbh->selectrow_array("SELECT username FROM predict.claim WHERE file = ?", undef, $safe_file_id);
	error() if (!defined $username);

	$dbh->do("INSERT INTO predict.session (cookie, username, started) VALUES (?, ?, now())", undef, $cookie, $username);
	$dbh->commit();
	

	$orig_hash{'owner'} = $hash{'username'};
}

# We copy only what we want to honor from the submitted json to the 'orig_json'

$orig_hash{'name'} = $hash{'name'};
$orig_hash{'description'} = $hash{'description'};

for (my $i=0; $i<$ncol; $i++)
{
	$orig_hash{'column'}[$i]{'name'} = $cols[$i]{'name'};
	$orig_hash{'column'}[$i]{'predict'} = $cols[$i]{'predict'};
	$orig_hash{'column'}[$i]{'dataType'} = $cols[$i]{'dataType'};

	if ($orig_hash{'column'}[$i]{'name'} =~ /^\*/)
	{
		$orig_hash{'column'}[$i]{'name'} =~ s/^\*//;
		$orig_hash{'column'}[$i]{'predict'} = 1; 
	} 
}


sub burp {
    my( $file_name ) = shift ;
    open( my $fh, ">$file_name" ) || 
        die "can't create $file_name $!" ;
    print $fh @_ ;
}

my $output = objToJson(\%orig_hash);

burp ($data_path, $output);

print "$output";
