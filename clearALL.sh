#!/bin/sh

rm -f upload/*
rm -f results/*.json
rm -f data/*.json
rm -f data/results/*.json

echo "DELETE FROM predict.file;" | psql 
echo "DELETE FROM predict.results;" | psql 
echo "DELETE FROM predict.attempts;" | psql 
echo "DELETE FROM predict.claim;" | psql 
