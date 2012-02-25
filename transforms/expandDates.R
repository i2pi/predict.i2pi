myTransform <- function (x)
{
	if (is.null(dim(x)))
	{
		if (class(x) != "Date") return (x);
		y <- data.frame(x)
		y[,2:3] <- cbind(weekdays(x), months(x))
		return(y)
	}

	numeric_idx <-  which(sapply(x[1,],function(d) class(d) == "Date"))
	y <- x
	nc <- ncol(y)
	
	for (j in 1:length(numeric_idx))
	{
		i <- numeric_idx[j]
		y[,2*(j-1)+nc+1] <- weekdays(y[,i])	
		y[,2*(j-1)+nc+2] <- months(y[,i])	

	}
	return(y)
}
