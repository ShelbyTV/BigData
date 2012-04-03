#!/bin/bash

userID=$(grep "twitter $1\$" ../userMapFile | awk '{print $1;}')

echo "=================================="

echo "$1 => $userID"

videoIDs=$(egrep "^$userID," ../broadcasts_unique.csv | awk -F, '{print $2;}')
numShares=$(echo $videoIDs | wc -w)

echo "$numShares videos shared"

echo "=================================="
echo ""

youtubeIDs=""

for videoID in $videoIDs; do
   youTubeID=$(egrep "^$videoID " ../videoMapFile | grep youtube | awk '{print $4;}')
   if [ "$youTubeID" == "" ]; then
      continue
   fi

   echo "=================================="
   echo "$youTubeID: $(curl -s "http://www.youtube.com/oembed?url=http%3A//wwube.com/watch%3Fv%3D$youTubeID" | sed 's#.*"title": "\([^"]*\)".*#\1#g')"

   egrep "^$videoID\s" all_parts_sorted_no_e | sort -r -n -k 3 | head -n 5 |  while read relatedLine; do
      relatedVal=$(echo $relatedLine | awk '{print $3;}')
      relatedVid=$(echo $relatedLine | awk '{print $2;}')
      
      relatedYouTubeID=$(egrep "^$relatedVid " ../videoMapFile | grep youtube | awk '{print $4;}')
      if [ "$relatedYouTubeID" == "" ]; then
         continue
      fi

      relatedYouTubeTitle=$(curl -s "http://www.youtube.com/oembed?url=http%3A//wwube.com/watch%3Fv%3D$relatedYouTubeID" | sed 's#.*"title": "\([^"]*\)".*#\1#g')

      echo "   $relatedVal $relatedYouTubeID: $relatedYouTubeTitle"
    
   done

   echo "=================================="
   echo ""

done
