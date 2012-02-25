#include <stdio.h>
#include <stdlib.h>

#include "data.h"

void	meta_json (FILE *fp, frameT *f)
{
	int	i,j;

	fprintf (fp, "{\n");
	fprintf (fp, "	\"name\":null,");
	fprintf (fp, "	\"owner\":null,");
	fprintf (fp, "	\"size\":%ld,\n", f->csv->size);
	fprintf (fp, "	\"estRows\":%ld,\n", f->csv->est_rows);
	fprintf (fp, "	\"column\": [\n");
	for (i=0; i<f->columns; i++)
	{
		columnT	*c = f->column[i];
		statsT	*s = &c->orig_stats;
		fprintf (fp, "		{\"dataType\": \"%s\", \"name\": \"%s\", \"predict\": %c,\n",
					c->type.name,
					c->name,
					c->predict ? '1' : '0');
		fprintf (fp, "		 \"stats\": { \"min\":%f, \"max\":%f, \"mean\":%f, \"stddev\":%f, \"quartiles\":[%f,%f,%f],\n",
						s->min, s->max, s->mean, s->stddev,
						s->quartile[0], s->quartile[1], s->quartile[2]);
		fprintf (fp, "		\"histogram\":{\"bins\": [\n");
		
		for (j=0; j<s->histogram.bins; j++)
		{
			char	str[1024];
			c->type.to_string (&c->type, &s->histogram.breaks[j], str);
			fprintf (fp, "\"%s\"%c", str, (j < s->histogram.bins-1) ? ',' : ' ');
		}
		fprintf (fp, "		], \"counts\": [\n");

		for (j=0; j<s->histogram.bins; j++)
		{
			fprintf (fp, "%ld%c", s->histogram.counts[j], (j < s->histogram.bins-1) ? ',' : ' ');
		}

		fprintf (fp, "]}}}%c\n", (i < f->columns-1) ? ',' : ' ');
	}
	fprintf (fp, "	]\n");
	fprintf (fp, "}\n");
}

int main (int argc, char **argv)
{
	frameT	*frame;
	FILE	*out;
	char	out_name[1024];

	if (argc != 2)
	{
		fprintf (stderr, "%s file.csv\n", argv[0]);
		exit (-1);
	}

	nlo_init();
	init_types();

	snprintf (out_name, 1020, "%s.json", argv[1]);
	out = fopen (out_name, "w");
	if (!out)
	{
		fprintf (stderr, "Failed to write to '%s'\n", out_name);
		exit (-2);
	}

	fp_error_out = out;

	frame = read_csv (argv[1]);	

	meta_json (out, frame);

	fclose (out);

	return (0);
}
