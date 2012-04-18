#!/usr/bin/python

import json
import codecs
import sys

inputFile = sys.stdin
broadcastCsvFile = codecs.open("broadcasts.csv", "w", "utf-8")
videoMapFile = codecs.open("videoMapFile", "w", "utf-8")
userMapFile = codecs.open("userMapFile", "w", "utf-8")

numVideos = 0
numUsers = 0

videos = {}
users = {}

for line in inputFile:
   data = json.loads(line)

   if (not (("%s %s" % (data["r"], data["s"])) in videos)):
      numVideos += 1
      videos[("%s %s" % (data["r"], data["s"]))] = numVideos
      videoMapFile.write("%d => %s %s\n" % (numVideos, data["r"], data["s"]))
      
   if (not (("%s %s" % (data["A"], data["x"])) in users)):
      numUsers += 1
      users[("%s %s" % (data["A"], data["x"]))] = numUsers
      userMapFile.write("%d => %s %s\n" % (numUsers, data["A"], data["x"]))
   
   videoNum = videos[("%s %s" % (data["r"], data["s"]))]
   userNum = users[("%s %s" % (data["A"], data["x"]))]
      
   broadcastCsvFile.write("%d,%d\n" % (userNum, videoNum))
