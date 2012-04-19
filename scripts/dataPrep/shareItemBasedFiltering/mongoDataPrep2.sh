#!/bin/bash

# nos@nos-db-s0-d:/tmp/bigdatatry1$ time mongoexport --port 27018 --db nos-production --collection broadcasts --fields A,x,v,r,s --csv 2>/dev/null | grep -v ",," | egrep "^\"facebook\"|^\"twitter\"|^\"tumblr\"" | grep -v ",$" | grep -v '""' | ~/bigdata_do_not_delete/BigData/bin/mongoDataPrep 
# Couldn't parse CSV line: 
# 
# real	130m48.705s
# user	66m37.740s
# sys	61m20.430s

mongoexport --port 27018 --db nos-production --collection broadcasts --fields A,x,v,r,s --csv 2>/dev/null | grep -v ",," | egrep "^\"facebook\"|^\"twitter\"|^\"tumblr\"" | grep -v ",$" | grep -v '""' | ./bin/mongoDataPrep 

# nos@nos-db-s0-d:~/bigdata_do_not_delete/4.18.2012$ time ../BigData/bin/csvPrune 
# 
# real	7m3.242s
# user	3m22.100s
# sys	3m39.400s


