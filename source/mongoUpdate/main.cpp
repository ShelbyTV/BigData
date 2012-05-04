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

typedef struct Video
{
   string site;
   string id;
} Video;

typedef enum MongoVideoStatus
{
   UNINITIALIZED = 0,
   UNAVAILABLE = 1,
   INITIALIZED = 2
} MongoVideoStatus;

typedef struct MongoVideo
{
   bson_oid_t mongoId;
   MongoVideoStatus status;
} MongoVideo;

typedef struct Recommendation
{
   unsigned int recId;
   double recVal;
} Recommendation;

static vector<Video> recVideoIDs;
static vector<MongoVideo> gtVideos;

static mongo conn;

static unsigned int mongoItemsWithRecommendations = 0;
static unsigned int mongoItemsTotalRecommendations = 0;

static struct Options {
   string outputFile;
   string videoMapFile;
} options; 

void printHelpText()
{
   cout << "recs usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -m --videomap    Video map file containing id to site/id translation (default: videos.csv)" << endl;
   cout << "   -o --output      Specify recommendations output file (default: output)" << endl;
}

bool connectToMongoAndAuthenticate()
{
   mongo_init(&conn);

   mongo_replset_init(&conn, "gtVideo");
   mongo_replset_add_seed(&conn, "gt-db-video-s0-a", 27017);
   mongo_replset_add_seed(&conn, "gt-db-video-s0-b", 27017);

   int status = mongo_replset_connect( &conn );

   if (MONGO_OK != status) {
      switch (conn.err) {
         case MONGO_CONN_SUCCESS:    break;
         case MONGO_CONN_NO_SOCKET:  printf("no socket\n"); return false;
         case MONGO_CONN_FAIL:       printf("connection failed\n"); return false;
         case MONGO_CONN_NOT_MASTER: printf("not master\n"); return false;
         default:                    printf("received unknown status\n"); return false;
      }
   }

   return (MONGO_OK == mongo_cmd_authenticate(&conn, "admin", "gt_admin", "Ov3rt1m3#4#"));
}

void loadVideoMap(const string &videoMapFileName)
{
   ifstream videoMapFile;
   string line;

   videoMapFile.open(videoMapFileName);

   while (videoMapFile.good()) {
      char site[1024];
      char id[1024];
      unsigned int item;

      getline(videoMapFile, line);

      if (3 != sscanf(line.c_str(), "%u,%[^,],%1023s", &item, site, id)) {
         continue;
      }

      if (item >= recVideoIDs.size()) {
         recVideoIDs.resize(2 * item);
      }
      
      recVideoIDs[item].site = site;
      recVideoIDs[item].id = id;
   }

   videoMapFile.close();
}

void getMongoVideoForItem(const unsigned int item)
{
   bson query;
   bson_init(&query);
   bson_append_string(&query, "a", recVideoIDs[item].site.c_str());
   bson_append_string(&query, "b", recVideoIDs[item].id.c_str());
   bson_finish(&query);
 
   bson fields;
   bson_init(&fields);
   bson_append_int(&fields, "_id", 1);
   bson_finish(&fields);

   bson out;
   bson_init(&out);

   if (mongo_find_one(&conn, "gt-video.videos", &query, &fields, &out) == MONGO_OK) {
      bson_iterator iterator;
      bson_iterator_init(&iterator, &out);
      gtVideos[item].mongoId = *bson_iterator_oid(&iterator);
      gtVideos[item].status = INITIALIZED;
   } else {
      gtVideos[item].status = UNAVAILABLE;
   }
 
   bson_destroy(&out);
   bson_destroy(&fields);
   bson_destroy(&query);
}

void getMongoVideoIfNeeded(const unsigned int item)
{
   if (gtVideos[item].status == UNINITIALIZED) {
      getMongoVideoForItem(item);
   } 
}

void printItemWithRecs(const unsigned int item, vector<Recommendation> &recs)
{
   cout << item << ":" << endl;
   for (unsigned int i = 0; i < recs.size(); i++) {
      cout << "   " << recs[i].recId << " " << recs[i].recVal << endl;
   }
}

void updateItemInMongoWithRecs(const unsigned int item, vector<Recommendation> &recs)
{
   static SimpleThrottle throttle(1000); // 1000 ops/sec

   bson cond; 
   bson_init(&cond);
   bson_append_oid(&cond, "_id", &gtVideos[item].mongoId);
   bson_finish(&cond);
   
   bson op;
   bson_init(&op);
   {
       bson_append_start_object(&op, "$set");
       bson_append_start_array(&op, "r"); 
       
       for (unsigned int i = 0; i < recs.size(); i++) {
         ostringstream stringStream;
         stringStream << i;

         bson_append_start_object(&op, stringStream.str().c_str());
         bson_append_oid(&op, "a", &gtVideos[recs[i].recId].mongoId);
         bson_append_double(&op, "b", recs[i].recVal);
         bson_append_finish_object(&op);
       }

       bson_append_finish_array(&op);
       bson_append_finish_object(&op);
   }
   bson_finish(&op);
  
   throttle.throttle(); 
   int status = mongo_update(&conn, "gt-video.videos", &cond, &op, 0);
   if (status != MONGO_OK) {
      cout << "ERROR updating video." << endl;
   }
 
   // bson_print(&cond); 
   // bson_print(&op);
 
   bson_destroy(&op);
   bson_destroy(&cond);
}

void addOrUpdateRecommendations(const string &recsFileName)
{
   ifstream recsFile;
   string line;
   unsigned int count = 0;
   vector<Recommendation> recs;
   unsigned int lastItem = numeric_limits<unsigned int>::max();   

   recsFile.open(recsFileName);

   while (recsFile.good()) {
      unsigned int item = 0, rec = 0;
      double val = 0;
     
      // if (count > 100) {
      //    break;
      // }

      if (count % 100000 == 0) {
         cout << count << endl;
      }

      getline(recsFile, line);

      if (3 != sscanf(line.c_str(), "%u %u %lf", &item, &rec, &val)) {
         continue;
      }

      if (recVideoIDs[item].site.empty() || recVideoIDs[item].id.empty() ||
          recVideoIDs[rec].site.empty() || recVideoIDs[rec].id.empty()) {
         continue;
      }

      count++;

      getMongoVideoIfNeeded(item);
      getMongoVideoIfNeeded(rec);

      if (item != lastItem) {
         if (recs.size() > 0) {
            // printItemWithRecs(lastItem, recs);
            updateItemInMongoWithRecs(lastItem, recs);

            mongoItemsWithRecommendations++;
            mongoItemsTotalRecommendations += recs.size();
         }
         lastItem = item;
         recs.clear();
      }   
    
      if (gtVideos[item].status == INITIALIZED && gtVideos[rec].status == INITIALIZED) { 
         Recommendation newRec;
         newRec.recId = rec;
         newRec.recVal = val;
         recs.push_back(newRec); 
      }
   }

   recsFile.close();
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",        no_argument,       0, 'h'},
         {"videomap",    required_argument, 0, 'm'},
         {"output",      required_argument, 0, 'o'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hm:o:", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'm':
            options.videoMapFile = optarg;
            break;
  
         case 'o':
            options.outputFile = optarg;
            break;

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
   options.outputFile = "output";
   options.videoMapFile = "videos.csv";
}

void printVideoStats()
{
   unsigned int mongoVideoIdsObtained = 0;
   unsigned int mongoVideoIdsDenied = 0;

   for (unsigned int i = 0; i < gtVideos.size(); i++) {
      if (gtVideos[i].status == INITIALIZED) {
         mongoVideoIdsObtained++;
      } else if (gtVideos[i].status == UNAVAILABLE) {
         mongoVideoIdsDenied++;
      }
   }

   cout << "Total mongo video IDs attempted: " << mongoVideoIdsObtained + mongoVideoIdsDenied << endl;
   cout << "Mongo video IDs obtained: " << mongoVideoIdsObtained << endl;
   cout << "Mongo video IDs denied: " << mongoVideoIdsDenied << endl;
   cout << endl;
   cout << "Total mongo videos with recommendations: " << mongoItemsWithRecommendations << endl;
   cout << "Total recommendations for all mongo videos: " << mongoItemsTotalRecommendations << endl;
}

void printMissingVideos()
{
   for (unsigned int i = 0; i < gtVideos.size(); i++) {
      if (gtVideos[i].status == UNAVAILABLE) {
         cout << recVideoIDs[i].site << "," << recVideoIDs[i].id << endl;
      }
   }
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

   // cout << "Connected to mongo." << endl;

   loadVideoMap(options.videoMapFile);

   // cout << "Video map loaded." << endl;

   gtVideos.resize(recVideoIDs.size());

   addOrUpdateRecommendations(options.outputFile); 

   // printVideoStats();
   // printMissingVideos();

mongoCleanup:
   mongo_destroy(&conn);
   return status;
}
