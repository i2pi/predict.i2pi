# requires(e1071) 

myModel <- function (m, d) svm (m, data=d, cost = 100, gamma = 1, kernel = 'linear')
myPredict <- predict
