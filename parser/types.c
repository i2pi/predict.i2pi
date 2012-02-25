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

typeT	type[MAX_TYPES];
int		types;

// TODO: Check for times following the date
char	*date_format[] = {"%Y-%m-%d", "%d/%m/%Y", "%m/%d/%Y", "%H:%M:%S", "%H:%M", "%M:%s"};
int		date_formats = 2;

unsigned char	nlo[256];	// lookup take for number of leaading ones

void	nlo_init (void)
{
	int				i;
	unsigned char	x;
	unsigned char	mask;

	for (i=0; i<256; i++)
	{
		x = i;
		mask = 1 << 7;
		nlo[i] = 0;
		while (mask && (x & mask)) { mask >>= 1; nlo[i]++; }
	}
}



void	date_from_string (struct typeT *self, char *str, double *x)
{
	struct tm t;

	if (!self->data)
	{
		// We use the data section to hold the format string.
		int	f = is_date(str);
		if (f == -1)
		{
			fprintf (stderr, "'%s' is not a Date\n", str);
			exit (-1);
		}

		self->data = (void *) strdup (date_format[f]);
	}

	memset (&t, 0, sizeof(struct tm));
	strptime (str, (char *) self->data, &t);
	*x  = (double) mktime (&t);
}

void	date_to_string (struct typeT *self, double *x, char *str)
{
	struct tm t;
	time_t	tt = (time_t) *x;
	localtime_r (&tt, &t);
	strftime(str, 100, (char *)self->data, &t);
}

void numeric_from_string (struct typeT *self, char *str, double *x)
{
	*x = atof(str);
}

void numeric_to_string (struct typeT *self, double *x, char *str)
{
	snprintf (str, 256, "%.6f", *x);
}

void integer_from_string (struct typeT *self, char *str, double *x)
{
	*x = atof(str);
}

void integer_to_string (struct typeT *self, double *x, char *str)
{
	snprintf (str, 256, "%ld", (long)*x);
}



void factor_to_string (struct typeT *self, double *x, char *str)
{
	unsigned long i = (unsigned long) *x;
	factorT	*f;

	f = (factorT *) self->data;
	if (i < f->items)
	{
		snprintf (str, 256, "%s", f->item[i]);
	} else
	{
		snprintf (str, 256, "NA!!");
	}
}

void factor_from_string (struct typeT *self, char *str, double *x)
{
	factorT	*f;
	int		i;

	if (!self->data)
	{
		f = (factorT *) malloc (sizeof(factorT));
		if (!f)
		{
			fprintf (stderr, "Failed to alloc factor\n");
			exit (-1);
		}
		f->items = 0;
		f->allocated = 32;
		f->item = (char **) malloc (sizeof (char *) * f->allocated);
		if (!f->item)
		{
			fprintf (stderr, "Failed to alloc factor list\n");
			exit (-1);
		}

		self->data = (void *) f;
	} else
	{
		f = (factorT *) self->data;
	}

	i = f->items - 1;
	while ((i >= 0) && strcmp(f->item[i], str)) i--;

	if (i < 0)
	{
		if (f->items >= f->allocated)
		{
			char	**new;
			f->allocated += (f->allocated >> 1) + 1;
			new = realloc (f->item, sizeof (char *) * f->allocated);
			if (!new)
			{
				fprintf (stderr, "Failed to realloc factor array\n");
				exit (-1);
			}
			f->item = new;
		}
		f->item[f->items] = strdup(str);
		if (!f->item[f->items])
		{
			fprintf (stderr, "Failed to strdup factor\n");
			exit (-1);
		}
		*x = (double) f->items;
		f->items++;
	} else
	{
		*x = (double) i;
	}
}



void	new_type (typeT *t, dtype type, char *name,
	              void (*from_string)(struct typeT *, char *, double *), 
	              void (*to_string)(struct typeT *, double *, char *))
{
	t->type = type;
	t->name = strdup(name);
	t->from_string = from_string;
	t->to_string = to_string;
	t->data = NULL; 
}

void	init_types (void)
{
	new_type(&type[Numeric], Numeric, "Numeric", numeric_from_string, numeric_to_string);
	new_type(&type[Integer], Integer, "Integer", integer_from_string, integer_to_string);
	new_type(&type[Binary], Binary, "Binary", integer_from_string, integer_to_string);
	new_type(&type[Factor], Factor, "Factor", factor_from_string, factor_to_string);
	new_type(&type[Date], Date, "Date", date_from_string, date_to_string);
}

int is_date (char *buf)
{
	struct tm	t;
	int			i;
	char		*end;	

	i = 0;
	while ( (i < date_formats) && !(end = strptime(buf, date_format[i], &t)) ) i++;
	if (end) return (i);

	return (-1);
}

char    *decomma (char *str)
{
    char    *cp,  *op;
    char    *out;

    out = strdup (str);
    cp = str;
    op = out;

    while (*cp != '\0')
    {
        if (*cp != ',')
        {
            *op++ = *cp;
        }
        cp++;
    }
    *op = '\0';

    return (out);
}



dtype guess_type (char *buf)
{
	int		len;
	char	*end;
	float	x;
	dtype	ret;
	
	len = strlen (buf);

	x = strtod (buf, &end);
	if (end != &buf[len])
	{
		char *no_commas = decomma(buf);
		x = strtod (no_commas, &end);
		len = strlen (no_commas);
		if (end != &no_commas[len])
		{
			// Not parsable as a float
			// Could be a date or a factor
			if (is_date(buf) != -1)
			{
				// Its a date
				ret = Date;
			} else
			{
				// A factor
				ret = Factor;
			}
		} else
		{
			if (floor(x) == x)
			{
				ret = Integer;
			} else
			{
				ret = Numeric;
			}
		}
	} else
	{
		if (floor(x) == x)
		{	
			ret = Integer;
		} else
		{
			ret = Numeric;
		}
	}

	return (ret);	
}
