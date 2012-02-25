#requires(rpart)

myModel <- function (formula, data) rpart(formula, data, na.action=na.exclude)
myPredict <- function (model, data)
{
	p <- predict (model, data)
	as.numeric(apply(p, 1, function(r) order(-r)[1]))
}
