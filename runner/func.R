source('/home/josh/www/predict/runner/measures.R')

initPaths <- function (base_dir)
{
	paths <- list();

	paths['meta'] <- paste (sep='', base_dir, 'data/')
	paths['data'] <- paste (sep='', base_dir, 'upload/')
	paths['model'] <- paste (sep='', base_dir, 'models/')
	paths['transform'] <- paste (sep='', base_dir, 'transforms/')
	paths['result'] <- paste (sep='', base_dir, 'results/')
	paths['unknown'] <- paste (sep='', base_dir, 'unknowns/')

	paths
}

toNum <- function (str) as.numeric(gsub(',', '', as.character(str)))

adjustColumns <- function (meta)
{
	cl.d <- sapply(colnames(meta$data), function(n) class(meta$data[,n]))
	cl.m <- tolower(sapply(meta$column, function(c) c$dataType))

	for (i in 1:ncol(meta$data))
	{
		if (cl.d[i] != cl.m[i])
		{
			if (cl.m[i] == 'numeric') meta$data[,i] <- toNum (meta$data[,i])
			if (cl.m[i] == 'integer') meta$data[,i] <- toNum (meta$data[,i])
			if (cl.m[i] == 'date') meta$data[,i] <- as.Date (meta$data[,i])
			if (cl.m[i] == 'factor') meta$data[,i] <- factor (meta$data[,i])
			if (cl.m[i] == 'binary') meta$data[,i] <- factor (meta$data[,i])
		}
	}

	colnames(meta$data) <- sapply(meta$column, function(c) c$name)

	meta
}

saveMeta <- function (db, meta, results)
{
	out_meta <- paste(sep='', paths$meta, meta$fileID, '.json')
	out_meta <- fromJSON(readChar(meta,nchars=128*1024))

}

loadData <- function(db, paths, fileID)
{
	meta <- paste(sep='', paths$meta, fileID, '.json')
	meta <- fromJSON(readChar(meta,nchars=128*1024))

	if (!is.null(meta$error))
	{
		print(paste ("Error File", fileID))
		stop()
	}

	meta$data <- read.csv(paste(sep='', paths$data, fileID))

	meta$data <- meta$data[sample(1:nrow(meta$data)),]

	meta$data <- meta$data[1:min( nrow(meta$data) , 10240),]	

	meta$fileID <- fileID

	meta
}


choosePrediction <- function (meta)
{
	predict <- (sapply(meta$column, function(c) c$predict) == 1)
	# if there is nothing to predict, then pick the last col
	# TODO: THIS IS BROKEN. NEEDS TO HAPPEN IN meta DURING loadData()
	if (all(!predict)) predict[length(predict)] <- T

	not_predict <- colnames(meta$data)[!predict]
	predict <- colnames(meta$data)[predict]

	response <- sample(predict)[1]

	# Remove any unknown outcomes and put them in a seperate data frame

	idx <- is.na(meta$data[,response])
	meta$unknownData <<- meta$data[idx,]
	meta$data <<- meta$data[!idx,]
	
	# Remove any rows from the data that contains NA's
#	idx <- apply(meta$data,1,function(r) !any(is.na(r)) )
#	meta$data <<- meta$data[idx,]
#	now handled by transform/ignoreNA.R

	response
}

chooseAlgo <- function (paths)
{
	m <- dir(paths$model)
	m <- m[m != 'disabled']
	algo <- sample (m, 1)
	algoName <- gsub(".R$", "", algo)
	algo <- paste(sep="/", paths$model, algo)
	source(algo)
	return (algoName)
}

chooseTransform <- function (paths)
{
	t <- dir(paths$transform)
	t <- t[t != 'disabled']
	transform <- sample (t, 1)
	transformName <- gsub(".R$", "", transform)
	transform <- paste(sep="/", paths$transform, transform)
	source(transform)
	return (transformName)
}

initResults <- function (fileID, predict)
{
	results <- list()
	results$fileID <- fileID
	results$calculated <- date()
	results$responseVariable <- predict
	results$train <- list()
	results$test <- list()

	results
}

applyTransform <- function (response, data)
{
	not_response_idx <- !(colnames(data) %in% response)
	t_data <- data
	t_data[,not_response_idx] <- inputTransform (t_data[,not_response_idx])

	t_data[,!not_response_idx] <- responseTransform (t_data[,!not_response_idx])

	t_data
}

chooseMeasure <- function (meta, response)
{
	cl.m <- tolower(sapply(meta$column, function(c) c$dataType))
	responseType <- cl.m[colnames(meta$data) == response][1]
	appropriateMeasures <- sapply(lapply(measures, function(m) m$types), function(t) responseType %in% t)
	idx <- which(appropriateMeasures)[1]
	print(paste(responseType, ' => ', names(idx)))
	return (list(name = names(idx), func=measures[[idx]]$'function'))
}

isModelError <- function (m)
{
	if (length(m) == 1)
	{
		if (class(m) == 'try-error')
		{
			return (TRUE);
		}
	}
	return (FALSE);
}

evaluateAlgo <- function (response, data, model_object, measure_func)
{
	m <- model_object 

	sN <- min(300, nrow(data))

	idx <- sample (1:nrow(data), sN)

	ret<-list()
	ret$predicted <- as.numeric( myPredict(m, data) ) 
	ret$actual <- data[,response]
	ret$measure <- measure_func(ret$actual, ret$predicted)

	idx <- sample (1:nrow(data), sN)
	ret$predicted <- ret$predicted[idx]
	ret$actual <- ret$actual[idx]

	return (ret)
}

toFormula <- function (meta, response)
{
	v <- c(response, colnames(meta$data)[colnames(meta$data) != response])
	v <- paste('`', v, '`', sep='')
	f <- paste (sep = ' ~ ', v[1], paste(v[2:length(v)], collapse = " + "))
	formula (f)
}

runTrials <- function (meta, response, measure_func)
{
	results <- initResults (meta$fileID, response)

	colnames(meta$data) <- gsub(' ', '.', colnames(meta$data))	
	response <- gsub(' ', '.', response)

	model <- toFormula (meta, response)

	Ntrain <- round(nrow(meta$data) * 0.5)
	start_time <- Sys.time()
	max_duration <- 15 * 60
	max_runs <- 25

	i <- 1
	while (difftime (Sys.time(), start_time, units="secs") < max_duration & i < max_runs)
	{
		print (paste ("Run #", i, ' Rows:', nrow(meta$data)))
		i <- i + 1
		data <- meta$data[sample(1:nrow(meta$data)),]

		train_data <- applyTransform (response, data[1:Ntrain,])
		m <- try (myModel (model, train_data))
	    train <- try(evaluateAlgo (response, train_data, m, measure_func))
		if (class(train) != "try-error")
		{
	    	results$train$measure <- c(results$train$measure, train$measure)
		}

		test_data <- applyTransform (response, data[(Ntrain+1):nrow(data),])
	    test  <- try(evaluateAlgo (response, test_data, m, measure_func))
		if (class(test) != "try-error") 
	    {
			# can error if the number of levels in a regressor changes
			results$test$measure <- c(results$test$measure, test$measure)
		}
		print (paste ("Running for :", as.integer(difftime (Sys.time(), start_time, units="secs"))))
	}
	results$nTrials <- i

	results$test$actual <- as.numeric(test$actual)
	results$test$predicted <- as.numeric(test$predicted)
	q <- quantile(results$test$measure, na.rm=T)
	names(q) <- paste (sep="", "q", sub("%","",names(q)))
	results$test$measure <- q

	results$train$actual <- as.numeric(train$actual)
	results$train$predicted <- as.numeric(train$predicted)
	results$train$predicted[is.na(results$train$predicted)] <- mean(results$train$predicted)
	q <- quantile(results$train$measure, na.rm=T)
	names(q) <- paste (sep="", "q", sub("%","",names(q)))
	results$train$measure <- q

	results$levels <- levels (meta$data)

	results$unknownData <- NULL
	if (nrow(meta$unknownData) > 0)
	{
		# If there are unknown responses, find our best predictions and 
		# store them separately
		unk_data <- applyTransform (response, meta$unknownData)
		results$unknownData <- meta$unknownData
		unk_data[,response] <- levels(meta$data[,response])[1]	# to kill NA's
		myp <-  try( myPredict(m, unk_data) )
		
		if (class(myp) != 'try-error')
		{	
			results$unknownData[,response] <-  myp
		}
	}
	

	results
}



tweet <- function(status){
  opts = curlOptions(header = FALSE, userpwd = "i2predict:pr3dict1027", netrc = FALSE)
  method <- "http://twitter.com/statuses/update.xml?status="
  encoded_status <- URLencode(status)
  request <- paste(method,encoded_status,sep = "")
	print(request)
  foo <- postForm(request,.opts = opts)
}



