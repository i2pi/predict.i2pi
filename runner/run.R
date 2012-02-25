#!/usr/local/bin/Rscript

library (rjson)
library (RODBC)
library (getopt)
library (RCurl)

library (rpart)
library (MASS)
library (class)
library (e1071)

source ('/home/josh/www/predict/runner/func.R')

spec <- c(
	"fileID", "f", 2, "character"
)
spec <- matrix(spec, ncol=4)
opts <- getopt (spec)

#opts$fileID <- 'V04ZlLEw' 


db <- odbcConnect("josh")
paths <- initPaths ('/home/josh/www/predict/')
meta <- loadData (db, paths, opts$fileID)
meta <- adjustColumns (meta)
response <- choosePrediction (meta)
model <-  formula(paste (sep="'", "", response," ~ ."))
measure <- chooseMeasure(meta, response)
measureName <- measure$name
algoName <- chooseAlgo(paths)

transformName <- chooseTransform(paths)
inputTransform <- myTransform

responseTransformName <- chooseTransform(paths)
responseTransform <- myTransform

rm(myTransform)


outname <- paste(sep="_", meta$fileID, responseTransformName, response, algoName, transformName, measureName, round(runif(1)*999999))
outpath <- paste(sep="", paths$result, outname, '.json')
unk_outpath <- paste(sep="", paths$unknown, outname, '.csv')
odbcClose(db)
print (paste("BEGIN:", outname))

db <- odbcConnect("josh")
sql <- paste(sep='', "INSERT INTO predict.attempts (file_id, model, transform, response_transform, calculated, response, measure_name) VALUES ('", paste(sep="\',\'", meta$fileID, algoName, transformName, responseTransformName, "now()", response, measureName ), "')" )
sqlQuery(db, sql)
odbcClose(db)

results <- runTrials (meta, response, measure$func)
results$modelName <- algoName
results$transformName <- transformName
results$responseTransofmName <- responseTransformName
results$measureName <- measureName

if (!is.null(results$unknownData))
{
	write.csv(results$unknownData, unk_outpath, row.names=F)
	results$unknownData <- paste(sep='', outname, '.csv')
}

output <- toJSON (results)
writeChar(output, outpath)

db <- odbcConnect("josh")
median_measure <- median(results$test$measure, na.rm=T)
sql <- paste(sep='', "INSERT INTO predict.results (file_id, model, transform, response_transform, calculated, response, measure, json, measure_name) VALUES ('", paste(sep="\',\'", meta$fileID, algoName, transformName, responseTransformName, results$calculated, response, median_measure, outname, measureName), "')" )
sqlQuery(db, sql)
best_measure<-sqlQuery(db, paste(sep='', "SELECT MAX(measure) FROM predict.results WHERE measure is NOT NULL AND file_id ='", meta$fileID, "'"))
odbcClose(db)

if (is.na(meta$name)) meta$name <- meta$fileID

if (median_measure > best_measure)
{
	tweet(paste ("New Results:", meta$name, paste(sep="", "http://predict.i2pi.com/", meta$fileID), repsonseTransform, '(', response, ')', transformName, algoName, ". ", measureName, " = ", round(median_measure, 2)))
}

print (paste("END:", outname))
q(status=1)
