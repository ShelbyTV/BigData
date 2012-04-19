#!/bin/bash

mongoexport --port 27018 --db nos-production --collection broadcasts --fields A,x,v,r,s --csv 2>/dev/null | grep -v ",," | egrep "^\"facebook\"|^\"twitter\"|^\"tumblr\"" | grep -v ",$" | grep -v '""' | ./bin/mongoDataPrep 
