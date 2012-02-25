#!/usr/bin/perl 

use strict;
use warnings;

use JSON;

use DBI;

my $dbh = DBI->connect('dbi:Pg:dbname=josh', '', '', {RaiseError => 1, PrintError=>1, AutoCommit => 0});


use Data::Dumper;

my $fileID = $ARGV[0];

my $base_dir = "/home/josh/www/predict";
my $data_dir = "$base_dir/data";
my $result_dir = "$base_dir/results";

my $query = qq{
	SELECT json 
	FROM
	   	(SELECT DISTINCT ON(model, transform, response) *
	   	 FROM predict.results
	   	 WHERE file_id = ?
   		 ORDER BY model, transform, response, measure DESC) g
	ORDER BY measure DESC LIMIT 18;
};

my $x = $dbh->selectall_arrayref ($query, undef, $fileID);
my @arr = @$x;

my @out;

foreach my $r (@arr)
{
	my ($json_path) = @$r;
	print "Loading $json_path.json\n";
	$json_path = $result_dir . "/" . $json_path . ".json";
	my $json;
	{
	    local( $/, *FH ) ;
	    open( FH, $json_path ) or die ("invalid file id");
	    $json= <FH>;
		close (FH);
	}
	$json =~ s/\0//g;
	push @out, $json;
}

open (FH, ">$data_dir/results/$fileID.json");
my $str = "[" . join(",", @out) . "]\n";
print FH $str;
close (FH);
