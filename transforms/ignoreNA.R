myTransform <- function (x)
{
	if (is.null(dim(x))) return (x);
	if (ncol(x) == 1) return (x);

	# Get rid of columns that have more than 50% NA's
	bad_idx <-  apply(x,2,function(c) sum(is.na(c)) / length(c)) >= 0.5
	if (any(bad_idx))
	{
		y <- x
		y[,bad_idx] <- is.na(y[,bad_idx])*1	# replace NA's with an indicator variable
		# remove rows with NA's
		# y <-y[apply(y,2,function(c) !any(is.na(c))),]
		return (y)
	} else
	{
		return (x)
	}
}
