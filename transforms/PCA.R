myTransform <- function (x)
{
	if (is.null(dim(x))) return (x)
	if (ncol(x) == 1) return (x)

	numeric_idx <-  sapply(x[1,],is.numeric)
	nx <- x[,numeric_idx]
	y <- x
	pc <- princomp(nx)$scores
	y[,numeric_idx] <- pc
	colnames(y)[numeric_idx] <- colnames(pc)
	if (ncol(y) > 20)
	{
		y <- y[,1:20]
	}
	y
}
