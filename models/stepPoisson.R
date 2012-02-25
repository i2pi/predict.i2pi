myModel <- function(...) step(glm(..., na.action=na.exclude, family='poisson'), trace=0, steps=5)
myPredict <- predict
