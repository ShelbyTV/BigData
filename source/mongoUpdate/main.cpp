#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include "lib/mongo-c-driver/src/mongo.h"

using namespace std;

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

static vector<Video> recVideoIDs;
static vector<MongoVideo> gtVideos;

static mongo conn;

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
   mongo_cursor cursor;

   bson_init(&query);
   bson_append_string(&query, "a", recVideoIDs[item].site.c_str());
   bson_append_string(&query, "b", recVideoIDs[item].id.c_str());
   bson_finish(&query);
 
   mongo_cursor_init(&cursor, &conn, "gt-video.videos");
   mongo_cursor_set_query(&cursor, &query);
 
   while(mongo_cursor_next(&cursor) == MONGO_OK) {
      bson_iterator iterator;
      bson_iterator_init(&iterator, mongo_cursor_bson(&cursor));
      gtVideos[item].mongoId = *bson_iterator_oid(&iterator);
      gtVideos[item].status = INITIALIZED;
   }
 
   bson_destroy(&query);
   mongo_cursor_destroy(&cursor);
}

void addOrUpdateRecommendations(const string &recsFileName)
{
   ifstream recsFile;
   string line;

   int count = 0;

   recsFile.open(recsFileName);

   while (recsFile.good()) {
      unsigned int item = 0, rec = 0;
      double val = 0;

      if (count > 10) {
         break;
      }

      getline(recsFile, line);

      if (3 != sscanf(line.c_str(), "%u %u %lf", &item, &rec, &val)) {
         continue;
      }

      if (recVideoIDs[item].site.empty() || recVideoIDs[item].id.empty()) {
         continue;
      }

      count++;


      if (gtVideos[item].status == UNINITIALIZED) {
         cout << "Item " << item << " is UNINITIALIZED." << endl;
         getMongoVideoForItem(item);
         cout << recVideoIDs[item].site << " " << recVideoIDs[item].id << " => ";

         if (gtVideos[item].status == INITIALIZED) {
            char temp[1024];
            bson_oid_to_string(&gtVideos[item].mongoId, temp);
            cout << temp << endl;
         } else {
            cout << "UNAVAILABLE" << endl; 
         }
      } else {
         cout << "Item " << item << " already processed." << endl;
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

int main(int argc, char **argv)
{
   int status = 0;

   setDefaultOptions();
   parseUserOptions(argc, argv);

   if (!connectToMongoAndAuthenticate()) {
      status = 1;
      goto mongoCleanup;
   } 

   cout << "Connected to mongo." << endl;

   loadVideoMap(options.videoMapFile);

   cout << "Video map loaded." << endl;

   gtVideos.resize(recVideoIDs.size());

   addOrUpdateRecommendations(options.outputFile); 

mongoCleanup:
   mongo_destroy(&conn);
   return status;
}
