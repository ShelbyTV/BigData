#!/usr/bin/env python
 
import sys
import re
import Stemmer
import string

stemmer = Stemmer.Stemmer('english')
table = string.maketrans("","")

# input comes from STDIN (standard input)
for line in sys.stdin:

    # remove leading and trailing whitespace
    line = line.strip()

    pat = r'"z" : "(?P<sharemessage>(.+?))"'
    result = re.search(pat, line)

    if result:
       values = result.groupdict()
       line = values["sharemessage"].strip()
       words = line.split()
       for word in words: 
          word = word.translate(table, string.punctuation)
          word = word.strip()
          
          if len(word) != 0 and not word.startswith("http"):
             print '%s\t%s' % (stemmer.stemWord(word.lower()), 1)
