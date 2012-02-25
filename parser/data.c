#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define __USE_XOPEN
#include <time.h>

#include "data.h"


frameT	*new_frame (char *name, int columns)
{
	frameT	*f;
	int		i;

	f = (frameT *) malloc (sizeof(frameT));
	if (!f)
	{
		fprintf (stderr, "Failed to allocate frame\n");
		exit (-1);
	}

	f->columns = columns;
	f->column = (columnT **) malloc (sizeof (columnT *) * f->columns);
	if (!f->column)
	{
		fprintf (stderr, "Failed to alloc columns\n");
		exit (-1);
	}
	for (i = 0; i<columns; i++)
	{
		f->column[i] = (columnT *) malloc (sizeof (columnT));
		if (!f->column[i])
		{
			fprintf (stderr, "Failed to alloc column %d\n", i);
			exit (-1);
		}
		f->column[i]->name = NULL;
		f->column[i]->transform = NULL;
		f->column[i]->orig_data = NULL;
	}
	f->rows = 0;

	f->region_rows = NULL;
	f->nn_distance = NULL;

	return (f);
}


void	init_column (frameT *f, int i, char *name, dtype t, char predict)
{
	if (f->column[i]->name) free (f->column[i]->name);
	f->column[i]->name = strdup(name);	

	if (!f->column[i]->name)
	{
		fprintf (stderr, "Failed to init column name '%s'\n", name);
		exit (-1);
	}

	memcpy (&f->column[i]->type, &type[t], sizeof (typeT));

	if (f->column[i]->transform) 
	{
		f->column[i]->transform = NULL;
	}	

	if (f->column[i]->orig_data)
	{
		f->column[i]->orig_data = NULL;
	}

	f->column[i]->frame = f;

	f->column[i]->predict = predict;
}

void	column_wipe_transforms (frameT *f, int i)
{
	columnT	*c = f->column[i];
	transformT	*t;

	for (t=c->transform; t; t = t->prev)
	{
		// Let the transform allocate space when needed
		free (t->data_out);
		t->data_out = NULL;
	}
}

void	column_realloc_data (frameT *f, int i, unsigned long rows)
{
	double	*new;
	columnT *c = f->column[i];

	new = (double *) realloc (c->orig_data, sizeof (double) * rows);
	if (!new)
	{
		fprintf (stderr, "Column realloc failed\n");
		exit (-1);
	}
	c->orig_data = new;

	column_wipe_transforms (f, i);
}

void	column_init_data (frameT *f, int i, unsigned long rows)
{
	columnT *c = f->column[i];

	if (c->orig_data)
	{
		free (c->orig_data);
	}
	c->orig_data = (double *) malloc (sizeof (double) * rows);
	if (!c->orig_data)
	{
		fprintf (stderr, "Failed to alloc column data\n");
		exit (-1);
	}

	memset (&c->orig_stats, 0, sizeof(statsT));
}

unsigned long rows_in_hist_range (histogramT *h, double min, double max)
{
	unsigned long	rows = 0;
	int				b;

	for (b=0; b < h->bins; b++) 
	{
		if ((h->breaks[b] >= min) && (h->breaks[b] <= max)) 
		{
			rows += h->counts[b];
		}
	}
	return rows;
}



unsigned long	find_min_idx_row_number (double min, double *data, unsigned long *idxs, unsigned long rows)
{	
	// Like bsearch, but returns the nearest thing it finds

	unsigned long 	l, u, i;
	double			val;

	l = 0;
	u = rows;
	while (l < u)
	{
		i = (l + u) / 2;

		val = data[idxs[i]];

		if (val > min)
			u = i;
		else if (val < min)
			l = i + 1;
		else return i;
	}
	return i;
}

unsigned long	find_max_idx_row_number (double max, double *data, unsigned long *idxs, unsigned long rows)
{	
	// Like bsearch, but returns the nearest thing it finds

	unsigned long 	l, u, i;
	double			val;

	l = 0;
	u = rows;
	while (l < u)
	{
		i = (l + u) / 2;

		val = data[idxs[i]];

		if (val > max)
			u = i;
		else if (val < max)
			l = i + 1;
		else return i;
	}
	return i;
}

void	nearest_neighbor (frameT *frame, int i, int j, double x, double y)
{
	int	k;
	double *X,*Y, dx, dy;

	// TODO: Reloaded data?

	if (!frame->nn_distance)
	{
		frame->nn_distance = (double *) malloc (sizeof(double) * frame->rows);
		if (!frame->nn_distance)
		{
			fprintf (stderr, "Unable to allocate nn distance\n");
			exit (-1);
		}
	}

	X = get_data(frame->column[i]);
	Y = get_data(frame->column[j]);

	dx = frame->column[i]->orig_stats.max - frame->column[i]->orig_stats.min;
	dy = frame->column[j]->orig_stats.max - frame->column[j]->orig_stats.min;

	for (k=0; k<frame->rows; k++)
	{
		frame->nn_distance[k] = pow(pow((X[k]-x)/dx,2.0) + pow((Y[k]-y)/dy,2.0), 0.25);
	}
}



void	mark_region (frameT *frame, int i, int j, double min_i, double max_i, double min_j, double max_j)
{
	// Mark all rows that belong to a rectangular region in the i by j plane
	// Rather than storing a tree for this, we use the sorted data from the stats to search

	// The way we do this is by looking at the histograms for column i and j and work out 
	// which of the two ranges are the most selective (i.e., eliminate the largest # of rows)
	// Once we find this, we bsearch to find our row indexes and then sequentially scan through
	// the other axis to eliminate the others.

	unsigned long	k;
	unsigned long 	i_rows, j_rows;
	unsigned long	min_idx, max_idx;
	statsT			*s;

	if (min_i > max_i)
	{
		double x;
		x = min_i;
		min_i = max_i;
		max_i = x;
	}
	
	if (min_j > max_j)
	{
		double x;
		x = min_j;
		min_j = max_j;
		max_j = x;
	}

	s = get_stats (frame->column[i]);
	i_rows = rows_in_hist_range (&s->histogram, min_i, max_i);

	s = get_stats (frame->column[j]);
	j_rows = rows_in_hist_range (&s->histogram, min_j, max_j);

	if (i_rows > j_rows)
	{
		int	t;	
		t = i;
		i = j;
		j = t;
		double	x;
		x = min_i;
		min_i = min_j;
		min_j = x;	
		x = max_i;
		max_i = max_j;
		max_j = x;	
	
	}

	// the i axis is most selective

	s = get_stats (frame->column[i]);
	min_idx = find_min_idx_row_number (min_i, get_data(frame->column[i]),
										s->sorted_row_numbers, frame->rows);
	max_idx = find_max_idx_row_number (max_i, get_data(frame->column[i]),
										s->sorted_row_numbers, frame->rows);

	memset (frame->region_rows, 0, frame->rows);

	for (k=min_idx; k<max_idx; k++)
	{
		unsigned long	n;
		double 			v;
		n = s->sorted_row_numbers[k];
		v = get_data(frame->column[j])[n];

		if ((v >= min_j) && (v <= max_j)) frame->region_rows[n] = 1;
	}
}

statsT  *get_stats (columnT *c)
{

	// TODO: For now, we only use the column stats as I'm not sure
	// if the performance gain is worth it. The original plan was to
	// store stats with each transformed data set, and in the case
	// of monotonic transforms, re-calc the stats based on the 
	// transform rather than going back to the underlying data.
	// In other words, can't be bothered for now...

	return &c->orig_stats;

/*
    if (c->transform)
    {
        return &(c->transform->stats_out);
    } else
    {
        return &c->orig_stats;
    }
*/
}

double	*get_data (columnT *c)
{
	if (c->transform)
	{
		fprintf (stderr, "NOT IMPLEMENTED IN PARSER\n");
		exit (-1);
	/*
		if (c->transform->data_out)
		{
			return c->transform->data_out;
		} else
		{
			//apply (c->transform);
			if (!c->transform->data_out)
			{
				fprintf (stderr, "Re-applied transform, but got no data back\n");
				exit (-1);
			} 
			return c->transform->data_out;
		}
	*/
	} else
	{
		return c->orig_data;
	}
}

void	show_stats (statsT *s)
{
	int	b, d;

	printf ("Range: %f, %f\n", s->min, s->max);
	printf ("Mean (sdev): %f (%f)\n", s->mean, s->stddev);
	printf ("Quartiles: %f, %f, %f\n", s->quartile[0], s->quartile[1], s->quartile[2]);
	printf ("Cardinality: %ld\n", s->cardinality);

	printf ("Histogram: \n");
	for (b=0; b<s->histogram.bins; b++)
	{
		printf ("%9.4f: ", s->histogram.breaks[b]);
		for (d=0; d<s->histogram.counts[b] * 20.0 / (double) s->histogram.max_count; d++) printf ("*");
		printf (" [%ld]\n", s->histogram.counts[b]);
	}
}

columnT	*evil_global_column;

int		row_cmp (const void *va, const void *vb)
{
	unsigned long ia = *(unsigned long *)va;
	unsigned long ib = *(unsigned long *)vb;
	double		  a, b;

	// TODO: Global variable use is to be frowned upon, but its friday night.
	a = get_data(evil_global_column)[ia];
	b = get_data(evil_global_column)[ib];
	if (a < b) return (-1);
	if (a > b) return ( 1);
	return (0);
}

void	update_column_stats (frameT *f, int col)
{
	columnT			*c = f->column[col];
	statsT			*s = &c->orig_stats;
	histogramT		*h = &s->histogram;
	unsigned long	i;
	double			prev_x;
	unsigned long	rows = f->rows;
	
	if (!s->sorted_row_numbers || (s->allocated_rows != rows))
	{
		if (s->sorted_row_numbers) free (s->sorted_row_numbers);
		s->sorted_row_numbers = (unsigned long *) malloc (sizeof(unsigned long) * rows);
		if (!s->sorted_row_numbers)
		{
			fprintf (stderr, "Failed to alloc data buffer for sorting\n");
			exit (-1);
		}
		s->allocated_rows = rows;
	}
	s->sorted_rows = 0;
	for (i=0; i<rows; i++) 
	{
		double	x = get_data(c)[i];
		if (!isnan(x) && !isinf(x))
		{
			s->sorted_row_numbers[s->sorted_rows++] = i;
		}
	}
	rows = s->sorted_rows;

	// First sort, as we need this for median, quantiles & histogram
	// TODO: Should merge in new things rather than always qsorting
	evil_global_column = c;
	qsort (s->sorted_row_numbers, rows, sizeof (unsigned long), row_cmp);

	s->min = get_data(c)[s->sorted_row_numbers[0]];
	s->max = get_data(c)[s->sorted_row_numbers[rows-1]];
	s->quartile[0] = get_data(c)[s->sorted_row_numbers[(int)(rows/4)]];
	s->quartile[1] = get_data(c)[s->sorted_row_numbers[(int)(rows/2)]];
	s->quartile[2] = get_data(c)[s->sorted_row_numbers[(int)(3*rows/4)]];
	
	s->sum = 0;
	s->sum_squares = 0;
	prev_x = s->min;
	s->cardinality = 1;
	for (i=0; i<rows; i++)
	{
		double x = get_data(c)[s->sorted_row_numbers[i]];

		s->sum += x;
		s->sum_squares += x * x;
		if (x != prev_x)
		{
			s->cardinality++;
			prev_x = x;
		}
	}
	s->mean = s->sum / (double) rows;
	s->stddev = sqrt((s->sum_squares / (double) rows) - (s->mean * s->mean));

	if (!h->bins)
	{
		int b;

		if (h->breaks) free (h->breaks);
		if (h->counts) free (h->counts);
		if (h->cdf)    free (h->cdf);

		if (s->cardinality > DEFAULT_HIST_BINS)
		{
			// Even spacing
			h->even_spaced = 1;
			h->bins = DEFAULT_HIST_BINS;
			h->breaks = (double *) malloc (sizeof (double) * h->bins);
			h->counts = (unsigned long *) malloc (sizeof (unsigned long) * h->bins);
			h->cdf = (double *) malloc (sizeof (double) * h->bins);
			if (!h->breaks || !h->counts || !h->cdf)
			{
				fprintf (stderr, "Failed to alloc histogram breaks\n");
				exit (-1);
			}	
			for (i=0; i<h->bins; i++)
			{
				h->breaks[i] = s->min + ((i+1) / (float) h->bins) * (s->max - s->min); 	
			}
			b = 0;
			memset (h->counts, 0, sizeof(unsigned long) * h->bins);
			h->max_count = 0;
			for (i=0; i<rows; i++)
			{
				double x = get_data(c)[s->sorted_row_numbers[i]];
				if ((x > h->breaks[b]) && (b < h->bins-1)) 
				{
					b++;
				}
				if (b >= h->bins) {printf ("HOLD YO HORSES |\n"); exit (-1);}
				h->counts[b]++;
				if (h->counts[b] > h->max_count) h->max_count = h->counts[b];
			}
		} else
		{
			// Uneven spacing - according to cardinality breaks
			h->even_spaced = 0;
			h->bins = s->cardinality;
			h->breaks = (double *) malloc (sizeof (double) * h->bins);
			h->counts = (unsigned long *) malloc (sizeof (unsigned long) * h->bins);
			h->cdf = (double *) malloc (sizeof(double) * h->bins);
			if (!h->breaks || !h->counts || !h->cdf)
			{
				fprintf (stderr, "Failed to alloc histogram breaks\n");
				exit (-1);
			}	
		
			b = 0;
			h->breaks[b] = s->min;
			h->counts[b] = 1;
			h->max_count = 0;
			for (i = 1; i<rows; i++)
			{
				double x = get_data(c)[s->sorted_row_numbers[i]];
				if (x > h->breaks[b]) 
				{
					h->breaks[++b] = x;
					h->counts[b] = 0;
				}
				if (b >= h->bins) {printf ("HOLD YO HORSES ||\n"); exit (-1);}
				h->counts[b]++;
				if (h->counts[b] > h->max_count) h->max_count = h->counts[b];
			}
		}

		h->cdf[0] = h->counts[0] / (double) f->rows;
		for (b=1; b < h->bins; b++)	h->cdf[b] = h->cdf[b-1] + (h->counts[b] / (double) f->rows);
	}
}

void	column_add_data (frameT *f, int i, char *str)
{
	// Given a string piece of data, will convert to the
	// binary data type and add the data to the end of the column

	columnT *c = f->column[i];
	double	*d = c->orig_data;

	c->type.from_string (&c->type, str, &d[f->rows]);
}

void column_pop_transform (frameT *f, int col)
{
	columnT	*c = f->column[col];

	if (!c->transform) return;

	transformT 	*t = c->transform;

	c->transform = t->prev;

	free(t->data_out);
	free(t->param);	

	c->orig_stats.histogram.bins = 0;
	update_column_stats  (f, col);
}

void column_add_transform (columnT *c, transformT *t)
{
	// Transforms are kept in a LIFO linked list

	if (c->transform) c->transform->next = t;
	t->prev = c->transform;
	t->next = NULL;
	t->column = c;
	c->transform = t;
	t->data_out = (double *) malloc (sizeof (double) * c->frame->rows);
	if (!t->data_out)
	{
		fprintf (stderr, "Failed to alloc data out\n");
		exit (-1);
	}
}

int	find_export_name (frameT *frame, char *name)
{
	// Name is assumed to be able to store 2048 bytes
	int			i;
	struct stat s;
	char		ext[] = ".smat.csv";

	snprintf (name, 2048, "%s%s", frame->csv->filename, ext);
	i=1;
	while (!stat (name, &s) && (i<1000))
	{
		snprintf (name, 2048, "%s%s.%d", frame->csv->filename, ext, i);
		i++;
	}
	if (errno != ENOENT)
	{
		perror("Trying to export frame");
		return (-1);
	} 

	if (i<1000) return (0);

	return (-1);
}


int	export_frame (frameT *frame, char *name)
{
	int	n, i;
	int ncol = frame->columns;
	columnT	*c ;
	FILE	*fp;

	i = find_export_name (frame, name);

	if (i) return (-1);

	fp = fopen (name, "w");
	if (!fp) return (-1);


	for (i=0; i<ncol; i++)
	{
		c = frame->column[i];
		fprintf (fp, "%s%c ", c->name, i < ncol-1 ? ',' : ' ');
	}
	fprintf (fp, "\n");

	for (n=0; n<frame->rows; n++)
	{
		for (i=0; i<ncol; i++)
		{
			char	str[256];
			double	d;

			d = get_data(frame->column[i])[n];

			c->type.to_string (&c->type, &d, str);
			fprintf (fp, "%s%c", str, i < ncol-1 ? ',' : ' ');
		}
		fprintf (fp,"\n");
	}

	fclose (fp);

	return (0);
}
