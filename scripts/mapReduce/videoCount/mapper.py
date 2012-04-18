#!/usr/bin/env python
 
import sys
import re

# input comes from STDIN (standard input)
for line in sys.stdin:

    # remove leading and trailing whitespace
    line = line.strip()

    pat = r'"r" : "(?P<source>(.+?))".*"s" : "(?P<sourceid>(.+?))"'
    result = re.search(pat, line)

    if result:
       values = result.groupdict()
       print '%s%s\t%s' % (values["source"], values["sourceid"], 1)
