myTransform <- function (x)
{
	if (is.null(dim(x)))
	{
		if (all(x) > 0) return (log(x))
		return (x)
	}

	numeric_idx <-  sapply(x[1,],is.numeric)
	nx <- x[,numeric_idx]
	y <- x
	y[,numeric_idx] <- apply(nx,2, function (d) if (all(d>0)) log(d) else d)
	colnames(y) <- colnames(x)
	return(y)
}
