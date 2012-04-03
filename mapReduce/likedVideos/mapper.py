#!/usr/bin/env python
 
import sys
import re

# input comes from STDIN (standard input)
for line in sys.stdin:

    # remove leading and trailing whitespace
    line = line.strip()

    pat = r'"nickname" : "(?P<nickname>(.+?))".*"liked_broadcasts" : \[(?P<liked>([^\]]+))\]'
    result = re.search(pat, line)

    if result:
       values = result.groupdict()
       
       objects = values["liked"].split(',')

       print '%s %s' % (len(objects), values["nickname"])
