myModel <- function (...) lm(..., na.action=na.exclude)
myPredict <- predict.lm
