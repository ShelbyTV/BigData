#!/bin/bash

# This script has not yet been tested. It's mostly here as a record of commands used to prepare
# mongo broadcast data to be processed by Amazon Elastic Map Reduce.
#
# Further optimizations for both latency and disk space / memory usage are probably possible.
#
# This should be run on a hidden replica machine.
#
# Example filesizes March 29, 2012
#
# -rw-r--r-- 1 nos nos 182G 2012-03-29 21:57 broadcasts.bson
# -rw-r--r-- 1 nos nos  11G 2012-03-29 23:01 broadcasts.json
# -rw-r--r-- 1 nos nos 9.3G 2012-03-29 23:20 broadcasts_all_fields.json
# -rw-r--r-- 1 nos nos 8.1G 2012-03-29 23:47 broadcasts_all_fields_uniq1.json
# -rw-r--r-- 1 nos nos 4.8G 2012-03-30 01:14 broadcastsPreProcessed.json
# -rw-r--r-- 1 nos nos 4.8G 2012-03-30 05:49 broadcastsPreProcessed2.json
# -rw-r--r-- 1 nos nos 4.8G 2012-03-30 05:55 broadcastsPreProcessed3.json
# -rw-r--r-- 1 nos nos 4.8G 2012-03-30 06:05 broadcastsPreProcessed4.json

mongodump --port 27018 --db nos-production --collection broadcasts --out .

# This uses a customized bsondump that supports data projections.
#
# https://github.com/mongodb/mongo/pull/66
# https://jira.mongodb.org/browse/SERVER-5453

../mongo/build/linux2/normal/mongo/bsondump -project '{ A: 1, r: 1, s:1, x:1, _id: 0}' broadcasts.bson > broadcasts.json

# This file is really large... Might be able to pipe into bsondump and never have to store it at all
rm broadcasts.bson 

# Some records don't have all the required fields, so get rid of them
awk '/\"A\"/&&/\"r\"/&&/\"s\"/&&/\"x\"/' broadcasts.json > broadcasts_all_fields.json

# Get rid of naturally repeated lines
uniq broadcasts_all_fields.json broadcasts_all_fields_uniq1.json

# Split the file, sort each piece and uniqify
split -l 1000000 broadcasts_all_fields_uniq1.json
for f in x*; do sort -u "$f" > "$f"_sorted; done
sort -um x*_sorted > broadcastsPreProcessed.json

# Get rid of null entries
grep -v ": null" broadcastsPreProcessed.json > broadcastsPreProcessed2.json

# Get rid of bookmarklet, search, extension, etc. and focus on real shares
awk '/\"twitter\"/||/\"facebook\"/||/\"tumblr\"/' broadcastsPreProcessed2.json > broadcastsPreProcessed3.json

# Get rid of blank values
grep -v ': \"\",' broadcastsPreProcessed3.json > broadcastsPreProcessed4.json

# Delete everything but broadcastsPreProcessed.json
rm x*
rm broadcasts.json
rm broadcasts_all*.json

