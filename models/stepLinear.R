myModel <- function(...) step(lm(..., na.action=na.exclude), trace=0, steps=5)
myPredict <- predict.lm
