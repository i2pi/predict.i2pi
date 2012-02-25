#!/bin/sh

echo Content-type: text/html
echo
echo "<h1>status</h1><pre>"

psql josh <<END_SQL
SELECT
    '<a href="http://predict.i2pi.com/' || file_id || '">' || file_id || '</a>',
	*
FROM predict.file
ORDER BY added DESC;


select
	f.added::date,
    a.file_id,
	a.response_transform,
    a.response,
    a.model,
    a.transform ,
    max(measure)::numeric(8,4) as measure,
    count(r.*) as runs
from predict.file f LEFT OUTER JOIN predict.attempts a ON 
	(f.file_id = a.file_id)
	LEFT OUTER JOIN predict.results r ON
    (r.file_id = a.file_id AND
     r.response = a.response AND
     r.model = a.model AND
     r.transform = a.transform AND
	 r.response_transform = a.response_transform )
group by f.added, a.file_id, a.model, a.response, a.transform, a.response_transform
order by f.added DESC, a.file_id, a.model, a.response, a.response_transform, a.transform
LIMIT 100;

select
    a.model,
    COALESCE((measure * 10)::INTEGER / 10.0,-1)::numeric(6,2) AS measure_bin,
    count(a.*) as attempts,
    (sum(coalesce(measure,-1)) / count(a.*))::numeric(6,2) as mean_measure
from predict.file f LEFT OUTER JOIN predict.attempts a ON
    (f.file_id = a.file_id)
    LEFT OUTER JOIN predict.results r ON
    (r.file_id = a.file_id AND
     r.response = a.response AND
     r.model = a.model AND
     r.transform = a.transform )
WHERE
    a.model IS NOT NULL AND
    a.model != 'disabled' AND
    a.transform != 'disabled'
group by a.model, measure_bin
order by a.model, measure_bin;

select
    a.transform,
    COALESCE((measure * 10)::INTEGER / 10.0,-1)::numeric(6,2) AS measure_bin,
    count(a.*) as attempts,
    (sum(coalesce(measure,-1)) / count(a.*))::numeric(6,2) as mean_measure
from predict.file f LEFT OUTER JOIN predict.attempts a ON
    (f.file_id = a.file_id)
    LEFT OUTER JOIN predict.results r ON
    (r.file_id = a.file_id AND
     r.response = a.response AND
     r.model = a.model AND
     r.transform = a.transform )
WHERE
    a.model IS NOT NULL AND
    a.model != 'disabled' AND
    a.transform != 'disabled'
group by a.transform, measure_bin
order by a.transform, measure_bin;


select
    a.model,
    a.transform,
    COALESCE((measure * 10)::INTEGER / 10.0,-1)::numeric(6,2) AS measure_bin,
    count(a.*) as attempts,
    (sum(coalesce(measure,-1)) / count(a.*))::numeric(6,2) as mean_measure
from predict.file f LEFT OUTER JOIN predict.attempts a ON
    (f.file_id = a.file_id)
    LEFT OUTER JOIN predict.results r ON
    (r.file_id = a.file_id AND
     r.response = a.response AND
     r.model = a.model AND
     r.transform = a.transform )
WHERE
    a.model IS NOT NULL AND
    a.model != 'disabled' AND
    a.transform != 'disabled'
group by a.model, a.transform, measure_bin
order by a.model, a.transform, measure_bin;

END_SQL
