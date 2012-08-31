#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <limits>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include "lib/mongo-c-driver/src/mongo.h"

using namespace std;

// NOTE: not threadsafe
class SimpleThrottle
{
public:
   SimpleThrottle(unsigned int opsPerSec)
   {
      assert(opsPerSec < 1000000);
      microSecondsBetweenOps.tv_sec = 0;
      microSecondsBetweenOps.tv_usec = 1000000 / opsPerSec; 
      setNextOpAllowed();
   }

   void throttle()
   {
      struct timeval currentTime;
      gettimeofday(&currentTime, NULL);

      if (timercmp(&currentTime, &nextOpAllowed, <)) {
         struct timeval difference;
         timersub(&nextOpAllowed, &currentTime, &difference);
         usleep(difference.tv_sec * 1000000 + difference.tv_usec);
      }
      setNextOpAllowed();
   } 

private:
   void setNextOpAllowed()
   {
      struct timeval currentTime;
      gettimeofday(&currentTime, NULL);
      timeradd(&currentTime, &microSecondsBetweenOps, &nextOpAllowed); 
   }

   struct timeval nextOpAllowed;
   struct timeval microSecondsBetweenOps;
};

static mongo conn_convo;
static mongo conn_frame;

void printHelpText()
{
   cout << "mongoConversationVideoFixer usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
}

bool printMongoConnectionErrorMessage(int status, mongo &conn)
{
   if (MONGO_OK != status) {
      switch (conn.err) {
         case MONGO_CONN_SUCCESS:    break;
         case MONGO_CONN_NO_SOCKET:  printf("no socket\n"); return false;
         case MONGO_CONN_FAIL:       printf("connection failed\n"); return false;
         case MONGO_CONN_NOT_MASTER: printf("not master\n"); return false;
         default:                    printf("received unknown status\n"); return false;
      }
   }

   return true;
}

bool connectToMongoAndAuthenticate()
{
   mongo_init(&conn_convo);
   mongo_init(&conn_frame);

   mongo_replset_init(&conn_convo, "gtConversation");
   mongo_replset_add_seed(&conn_convo, "gt-db-conversation-s0-a", 27017);
   mongo_replset_add_seed(&conn_convo, "gt-db-conversation-s0-b", 27017);

   int status_convo = mongo_replset_connect(&conn_convo);
   printMongoConnectionErrorMessage(status_convo, conn_convo);

   mongo_replset_init(&conn_convo, "gtRollFrame");
   mongo_replset_add_seed(&conn_convo, "gt-db-roll-frame-s0-a", 27017);
   mongo_replset_add_seed(&conn_convo, "gt-db-roll-frame-s0-b", 27017);

   int status_frame = mongo_replset_connect(&conn_frame);
   printMongoConnectionErrorMessage(status_frame, conn_frame);

   return (MONGO_OK == mongo_cmd_authenticate(&conn_convo, "admin", "gt_admin", "Ov3rt1m3#4#") &&
           MONGO_OK == mongo_cmd_authenticate(&conn_frame, "admin", "gt_admin", "Ov3rt1m3#4#"));
}

bson_oid_t getMongoVideoFromFrame(const bson_oid_t &frame)
{
   bson query;
   bson_init(&query);
   bson_append_oid(&query, "_id", &frame);
   bson_finish(&query);
 
   bson fields;
   bson_init(&fields);
   bson_append_int(&fields, "c", 1);
   bson_finish(&fields);

   bson out;
   bson_init(&out);

   bson_oid_t video;

   if (mongo_find_one(&conn_frame, "gt-frame.frames", &query, &fields, &out) == MONGO_OK) {
      bson_iterator iterator;
      bson_iterator_init(&iterator, &out);
      video = *bson_iterator_oid(&iterator);
   } else {
      exit(1);
   }
 
   bson_destroy(&out);
   bson_destroy(&fields);
   bson_destroy(&query);

   return video;
}

void updateConversationInMongoWithVideo(bson_oid_t convo, bson_oid_t video)
{
   static SimpleThrottle throttle(100); // 1000 ops/sec

   bson cond; 
   bson_init(&cond);
   bson_append_oid(&cond, "_id", &convo);
   bson_finish(&cond);
   
   bson op;
   bson_init(&op);
   {
       bson_append_start_object(&op, "$set");
       bson_append_oid(&op, "a", &video);
       bson_append_finish_object(&op);
   }
   bson_finish(&op);
  
   throttle.throttle(); 
   int status = mongo_update(&conn_convo, "gt-conversation.conversations", &cond, &op, 0);
   if (status != MONGO_OK) {
      cout << "ERROR updating conversation." << endl;
   }
 
   bson_destroy(&op);
   bson_destroy(&cond);
}

void fixConversationsWithoutVideos()
{
   unsigned int total = 1;
   unsigned int fixed = 1;

   bson query;
   bson_init(&query);

   bson_append_start_object(&query, "$query");
   bson_append_null(&query, "a");
   bson_append_finish_object(&query);

   bson_finish(&query);

   bson fields;
   bson_init(&fields);
   bson_append_int(&fields, "c", 1);
   bson_finish(&fields);

   mongo_cursor cursor;
   mongo_cursor_init(&cursor, &conn_convo, "gt-conversation.conversations");
   mongo_cursor_set_query(&cursor, &query);
   mongo_cursor_set_fields(&cursor, &fields);

   while (mongo_cursor_next(&cursor) == MONGO_OK) {
      bson_iterator iterator_frame;
      bson_iterator iterator_convo;

      if (bson_find(&iterator_frame, mongo_cursor_bson(&cursor), "c") &&
          bson_find(&iterator_convo, mongo_cursor_bson(&cursor), "_id"))
      {
         bson_oid_t frame_id = *bson_iterator_oid(&iterator_frame);
         bson_oid_t convo_id = *bson_iterator_oid(&iterator_convo);

         bson_oid_t video_id = getMongoVideoFromFrame(frame_id);
         updateConversationInMongoWithVideo(convo_id, video_id);

         fixed++;
      }

      if (total % 1000 == 0) {
         cout << "Total: ~" << total << endl;
      }
      if (fixed % 1000 == 0) {
         cout << "Fixed: ~" << fixed << endl;
         fixed++;
      }

      total++;
   }
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",        no_argument,       0, 'h'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "h", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }
}

void setDefaultOptions()
{
}

int main(int argc, char **argv)
{
   int status = 0;

   setDefaultOptions();
   parseUserOptions(argc, argv);

   if (!connectToMongoAndAuthenticate()) {
      status = 1;
      goto mongoCleanup;
   } 

   fixConversationsWithoutVideos();

mongoCleanup:
   mongo_destroy(&conn_convo);
   mongo_destroy(&conn_frame);
   return status;
}
