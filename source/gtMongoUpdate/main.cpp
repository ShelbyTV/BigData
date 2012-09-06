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
   bson_oid_t oid;
} Video;

typedef struct Recommendation
{
   unsigned int recId;
   double recVal;
} Recommendation;

static vector<Video> videoIDs;

static mongo conn;

static struct Options {
   string outputFile;
   string videoMapFile;
} options; 

void printHelpText()
{
   cout << "gtMongoUpdate usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -m --videomap    Video map file containing id to mongo id translation (default: videos.csv)" << endl;
   cout << "   -o --output      Specify recommendations output file (default: output)" << endl;
}

bool connectToMongoAndAuthenticate()
{
   mongo_init(&conn);

   mongo_replset_init(&conn, "gtVideo");
   mongo_replset_add_seed(&conn, "gt-db-video-s0-a", 27017);
   mongo_replset_add_seed(&conn, "gt-db-video-s0-b", 27017);

   int status = mongo_replset_connect(&conn);

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
      char id[1024];
      unsigned int item;

      getline(videoMapFile, line);

      if (2 != sscanf(line.c_str(), "%u,%1023s", &item, id)) {
         continue;
      }

      if (item >= videoIDs.size()) {
         videoIDs.resize(2 * item);
      }
      
      bson_oid_from_string(&videoIDs[item].oid, id);
   }

   videoMapFile.close();
}

void updateItemInMongoWithRecs(const unsigned int item, vector<Recommendation> &recs)
{
   static SimpleThrottle throttle(1000); // 1000 ops/sec

   bson cond; 
   bson_init(&cond);
   bson_append_oid(&cond, "_id", &videoIDs[item].oid);
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
         bson_append_oid(&op, "a", &videoIDs[recs[i].recId].oid);
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
     
      if (count % 100000 == 0) {
         cout << count << endl;
      }

      getline(recsFile, line);

      if (3 != sscanf(line.c_str(), "%u %u %lf", &item, &rec, &val)) {
         continue;
      }

      count++;

      if (item != lastItem) {
         if (recs.size() > 0) {
            updateItemInMongoWithRecs(lastItem, recs);
         }
         lastItem = item;
         recs.clear();
      }   
    
      Recommendation newRec;
      newRec.recId = rec;
      newRec.recVal = val;
      recs.push_back(newRec); 
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

int main(int argc, char **argv)
{
   int status = 0;

   setDefaultOptions();
   parseUserOptions(argc, argv);

   if (!connectToMongoAndAuthenticate()) {
      status = 1;
      goto mongoCleanup;
   } 

   loadVideoMap(options.videoMapFile);
   addOrUpdateRecommendations(options.outputFile); 

mongoCleanup:
   mongo_destroy(&conn);
   return status;
}
