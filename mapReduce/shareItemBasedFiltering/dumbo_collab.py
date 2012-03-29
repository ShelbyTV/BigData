import re
from dumbo import sumreducer, nlargestreducer, nlargestcombiner, main
from heapq import nlargest
from math import sqrt

class mapper1:
    def __init__(self):
        pat = r'"A" : "(?P<sharesource>(.+?))".*"r" : "(?P<source>(.+?))".*"s" : "(?P<sourceid>(.+?))".*"x" : "(?P<shareid>(.+?))"'
        self.matcher = re.compile(pat)
    
    def __call__(self, key, value):
        line = value.strip()
        result = self.matcher.search(line)
    
        if result:
           values = result.groupdict()
           video = '%s%s' % (values["source"], values["sourceid"])
           sharer = '%s%s' % (values["sharesource"], values["shareid"])
           yield (video, sharer), 1

def mapper2(key, value):
    yield key[0], (value, key[1])

def reducer2(key, values):
    values = nlargest(1000, values)
    norm = sqrt(sum(value[0]**2 for value in values))
    for value in values:
        yield (value[0], norm, key), value[1]

def mapper3(key, value):
    yield value, key

def mapper4(key, value):
    for left, right in ((l, r) for l in value for r in value if l != r):
        yield (left[1:], right[1:]), left[0]*right[0]

def mapper5(key, value):
    left, right = key
    yield left[1], (value / (left[0]*right[0]), right[1])

def runner(job):
    job.additer(mapper1, sumreducer, combiner=sumreducer)
    job.additer(mapper2, reducer2)
    job.additer(mapper3, nlargestreducer(1000), nlargestcombiner(1000))
    job.additer(mapper4, sumreducer, combiner=sumreducer)
    job.additer(mapper5, nlargestreducer(5), nlargestcombiner(5))

if __name__ == "__main__":
    main(runner)
