#!/usr/bin/python

import cjson
import codecs
import sys
import re

inputFile = sys.stdin
broadcastCsvFile = codecs.open("broadcasts.csv", "w", "utf-8")
videoMapFile = codecs.open("videoMapFile", "w", "utf-8")
userMapFile = codecs.open("userMapFile", "w", "utf-8")

numVideos = 0
numUsers = 0

videos = {}
users = {}

for line in inputFile:
   data = cjson.decode(line)

   try: 
      videoProvider = data["r"]
      videoId = data["s"]
      sharerSource = data["A"]
      sharerNickname = data["x"]
      sharerImage = data["v"]
   except:
      continue

   if (videoId == None):
      continue

   if (sharerSource != "facebook" and sharerSource != "twitter" and sharerSource != "tumblr"):
      continue
   
   if (sharerSource == "facebook"):
      pattern = r"http://graph.facebook.com/(?P<facebookId>([0-9]+?))/picture"
      try:
         result = re.search(pattern, sharerImage)
      except:
         continue

      if result:
         values = result.groupdict()
         sharerId = values["facebookId"]
      else:
         continue
   else:
      sharerId = sharerNickname

   if (sharerId == None or sharerId == ""):
      continue

   videoProviderIdString = "%s %s" % (videoProvider, videoId)

   if (not videoProviderIdString in videos):
      numVideos += 1
      videos[videoProviderIdString] = numVideos
      videoMapFile.write("%d => %s\n" % (numVideos, videoProviderIdString))
  
   userIdString = "%s %s" % (sharerSource, sharerId) 
   
   if (not userIdString in users):
      numUsers += 1
      users[userIdString] = numUsers
      userMapFile.write("%d => %s\n" % (numUsers, userIdString))
   
   videoNum = videos[videoProviderIdString]
   userNum = users[userIdString]
      
   broadcastCsvFile.write("%d,%d\n" % (userNum, videoNum))
