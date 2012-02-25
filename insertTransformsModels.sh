#!/bin/sh

/home/josh/www/predict/bin/watcher.sh |  grep INSERT | psql 2&>1 | grep -v "violates unique"
