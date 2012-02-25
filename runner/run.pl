#!/usr/bin/perl 

use strict;
use warnings;

use DBI;

my $dbh = DBI->connect('dbi:Pg:dbname=josh', '', '', {RaiseError => 1, PrintError=>0, AutoCommit => 0});

my $i = 0;
my $max = 4;

my ($fileID) = $dbh->selectrow_array (qq{
		SELECT file_id 
		FROM
		(
			select f.file_id, count(r.*) + 2*random() as attempts
	        from predict.file f left outer join predict.attempts r on 
					(r.file_id = f.file_id) 
			where f.parse_status = 0
			group by f.file_id
	        order by attempts
			LIMIT 5
		) q
		ORDER BY random() LIMIT 1;
	});
	
while ($i < $max)
{

	print "*****************************************************************************\n";
	print "RUNNING $fileID [$i / $max]\n";
	print "*****************************************************************************\n";
	system "/home/josh/www/predict/runner/run.R", "-f", "$fileID";
	system "/home/josh/www/predict/runner/updateResults.pl", "$fileID";
	$i++;
}


