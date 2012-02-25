#ifndef __DATA_H__
#define __DATA_H__
#include <stdio.h>
#include <stdlib.h>

#define MAX_TYPES 16
#define MAX_TRANSFORMS 64

#define MAX_CSV_LINE_LENGTH	(16384*16)
#define BITMAP_SIZE	(1024*1024*16)
#define DEFAULT_HIST_BINS	32

extern char *date_format[];
extern int   date_formats;

typedef struct csvT {
	char	*filename;
	FILE	*fp;
	unsigned long	size;
	unsigned int est_row_size;
	unsigned long est_rows;
	unsigned int bytes_per_bitmap_bit;
	unsigned char	*bitmap_loaded;
	unsigned char	*bitmap_display;	// Unused for now.
	unsigned long	bitmap_size;
} csvT;

typedef enum 
{
	Numeric = 0,
	Integer,
	Binary,
	Factor,
	Date
} dtype;

typedef struct factorT
{
	char			**item;
	unsigned long	items;
	unsigned long	allocated;
} factorT;

typedef struct typeT
{
	dtype		type;
	char		*name;
	void		(*from_string)(struct typeT *, char *, double *);
	void		(*to_string)(struct typeT *, double *, char *);
	void		*data;		
} typeT;

extern typeT	type[MAX_TYPES];
extern int		types;

extern unsigned char nlo[256];

typedef struct histogramT
{
	int				bins;
	double			*breaks;
	unsigned long	*counts;
	double			*cdf;
	unsigned long	max_count;
	char			even_spaced;
} histogramT;

typedef struct statsT
{
	double		max, min;
	double		mean;
	double		stddev;
	double		sum;
	double		sum_squares;
	double		quartile[3];
	histogramT	histogram;
	unsigned long	cardinality;

	unsigned long	*sorted_row_numbers;
	unsigned long	sorted_rows;		// may be less than frame->rows, as we only include the finite values
	unsigned long	allocated_rows;
} statsT;

typedef struct transformT
{
	char			*name;
	char			key;
	void			(*apply)(struct transformT *, double *, statsT *);
	char			is_monotonic;
	void			*param;
	size_t			param_sz;
	double			*data_out;
	statsT			stats_out;
	struct transformT	*next;
	struct transformT	*prev;
	struct columnT		*column;
} transformT;

extern transformT	transform[MAX_TRANSFORMS];
extern int			transforms;

typedef struct columnT 
{
	char		*name;
	char		predict;
	typeT		type;
	transformT	*transform;
	double		*orig_data;
	statsT		orig_stats;
	struct frameT	*frame;
} columnT;

typedef struct frameT
{
	csvT			*csv;
	unsigned long	rows;
	unsigned long	allocated_rows;
	columnT			**column;
	int				columns;

	unsigned long	allocated_region_rows; // only for debugging TODO - remove this
	unsigned char	*region_rows;		// for storing whether a row has been found to be in a region
	double			*nn_distance;		// for nearest neighbor gradients
} frameT;

int is_date (char *buf);
frameT	*read_csv (char *filename);

void	nlo_init (void);

frameT	*new_frame (char *name, int columns);
void	free_frame (frameT *f);
void    print_frame (frameT *frame);
int		export_frame (frameT *frame, char *name);
void	init_column (frameT *f, int i, char *name, dtype t, char predict);
void	column_init_data (frameT *f, int i, unsigned long rows);
void    column_realloc_data (frameT *f, int i, unsigned long rows);
void	column_add_data (frameT *f, int i, char *str);
void 	column_add_transform (columnT *c, transformT *t);
void 	column_pop_transform (frameT *f, int col);
void    column_apply_transforms (frameT *f, int col);
void    column_wipe_transforms (frameT *f, int i);
void    update_column_stats (frameT *f, int i);
void    show_stats (statsT *s);
void    load_random_rows (frameT *frame, float pct);

statsT  *get_stats (columnT *c);
double	*get_data (columnT *c);
void    get_transform_data (transformT *t, double **data, statsT **stats);

void	mark_region (frameT *frame, int i, int j, double min_i, double max_i, double min_j, double max_j);
void    nearest_neighbor (frameT *frame, int i, int j, double x, double y);

void	init_transforms (void);
transformT 	*clone_transform (transformT *t);

dtype	guess_type(char *buf);

extern FILE *fp_error_out;
void    parse_error (char *fname, char *msg, char *context);




void	init_types (void);

#endif /* __DATA_H__ */
