myTransform <- function (x)
{
	if (is.null(dim(x)))
	{
		return (order(x) / length(x))
	}

	if (ncol(x) > 1)
	{
		numeric_idx <-  sapply(x[1,],is.numeric)
		nx <- x[,numeric_idx]
		y <- x
		y[,numeric_idx] <- apply(nx,2, function(d) order(d) / length(d))
		return(y)
	} else
	{
		return (order(x) / length(x))
	}
}
