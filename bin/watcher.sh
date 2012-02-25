#!/bin/sh

# Watches for new models or transforms

BASE_PATH=/home/josh/www/predict

models=`find $BASE_PATH/models -type f -name '*.R' | grep -v disabled | awk -F/ '{print $NF}'`
xforms=`find $BASE_PATH/transforms -type f -name '*.R' | grep -v disabled | awk -F/ '{print $NF}'`

for n in $models
do
	SAFE=`echo $n | sed "s/[^A-Za-z0-9_\-\.]//g"`
	echo "INSERT INTO predict.models (name) VALUES ('$SAFE');"
done

for n in $xforms
do
	SAFE=`echo $n | sed "s/[^A-Za-z0-9_\-\.]//g"`
	echo "INSERT INTO predict.transforms (name) VALUES ('$SAFE');"
done
