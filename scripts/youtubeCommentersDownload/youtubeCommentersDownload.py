#!/usr/bin/python

import sys

def cfor(first,test,update):
    while test(first):
        yield first
        first = update(first)

def get_path():
    '''Get the path to this script no matter how it's run.'''
    #Determine if the application is a py/pyw or a frozen exe.
    if hasattr(sys, 'frozen'):
        # If run from exe
        dir_path = os.path.dirname(sys.executable)
    elif '__file__' in locals():
        # If run from py
        dir_path = os.path.dirname(__file__)
    else:
        # If run from command line
        dir_path = sys.path[0]
    return dir_path

currentPath = get_path()
sys.path.append(currentPath + '/../scripts/youtubeCommentersDownload/gdata-2.0.17/src/')

import gdata.youtube
import gdata.youtube.service
import gdata.opensearch.data

if (len(sys.argv) <= 1 or len(sys.argv) > 2):
    sys.exit("Usage: %s [youtube video id]" % (sys.argv[0]))

try:

   YOUTUBE_VIDEO_URI = 'https://gdata.youtube.com/feeds/api/videos'
   
   yt_service = gdata.youtube.service.YouTubeService()
   
   # developer key registered with mark@shelby.tv Google Account
   yt_service.developer_key = 'AI39si74MByVvkhznlNIHifwlMn_7SOdoo8MVuTcbN_GzxZ4nQOOl89id99XSH4scvOd3uNDxzzJYZyrGwUKqTYE8J2h8SxUUQ'
   
   video_id = sys.argv[1] # 'rYEDA3JcQqw'
   base_uri = '%s/%s/%s' % (YOUTUBE_VIDEO_URI, video_id, 'comments')
   
   uri = '%s?%s&%s' % (base_uri, 'start-index=1', 'max-results=50')
   comment_feed = yt_service.GetYouTubeVideoCommentFeed(uri=uri)
   total_comments = int(comment_feed.total_results.text)
   
   for i in cfor(1, lambda i : i <= 950 and i <= total_comments, lambda i : i + 50):
      
      if i > 1: 
         start_index = 'start-index=%d' % (i)
         uri = '%s?%s&%s' % (base_uri, start_index, 'max-results=50')
         comment_feed = yt_service.GetYouTubeVideoCommentFeed(uri=uri)
   
      for comment_entry in comment_feed.entry:
         for author_entry in comment_entry.author: 
            print author_entry.name.text

except Exception:

  sys.exit()
