#!/usr/bin/env python
 
import sys
import re

# input comes from STDIN (standard input)
for line in sys.stdin:

    # remove leading and trailing whitespace
    line = line.strip()

    pat = r'"A" : "(?P<sharesource>(.+?))".*"x" : "(?P<shareid>(.+?))"'
    result = re.search(pat, line)

    if result:
       values = result.groupdict()
       print '%s%s\t%s' % (values["sharesource"], values["shareid"], 1)
