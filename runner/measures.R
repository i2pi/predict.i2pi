measures <- list()

measures$'Accuracy' <- list()
measures$'Accuracy'$'function' <- function (actual, predicted)
{
	t <- table (actual, predicted)
	sum(diag(t)) / sum(t)
}
measures$'Accuracy'$types <- c('binary', 'factor')

measures$'R Squared' <- list()
measures$'R Squared'$'function' <- function (actual, predicted)
{
	cor (actual, predicted, use='pairwise.complete.obs')^2
}
measures$'R Squared'$types <- c('numeric', 'integer', 'date')


