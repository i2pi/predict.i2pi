library(RCurl)

urls<-list()
urls$upload <- 'http://predict.i2pi.com/upload.cgi'

data <- data.frame (matrix (runif(1000*8),nrow=1000,ncol=8))
data[,ncol(data)] <- ncol(data) * data[,ncol(data)] / 4  +  apply (data, 1, sum) 

write.csv(data, textConnection("csv", open="w"))
ret <- postForm (urls$upload, csv_file=csv)

