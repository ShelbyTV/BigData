#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lib/mongo-c-driver/src/mongo.h"

using namespace std;

static struct Options {
   string broadcastsFile;
   string videosFile;
   string usersFile;
} options; 

static mongo conn;

void printHelpText()
{
   cout << "mongoDataPrep usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -m --videomap    Output name for video map file containing id to site/id translation (default: videos.csv)" << endl;
   cout << "   -n --usermap     Output name for user map file containing id to network/id translation (default: users.csv)" << endl;
   cout << "   -o --output      Output name for broadcasts file (default: broadcasts.csv)" << endl;
}

static unsigned int uniqueVideosCount = 0;
static map<string, unsigned int> videoOids;

typedef enum rollType
{
   UNINITIALIZED = 0,
   GENIUS,
   OTHER
} rollType;

typedef struct Roll
{
   unsigned int id;
   rollType type;
} Roll;

static unsigned int uniqueRollsCount = 0;
static map<string, Roll> rolls;

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",        no_argument,       0, 'h'},
         {"videomap",    required_argument, 0, 'm'},
         {"usermap",     required_argument, 0, 'n'},
         {"output",      required_argument, 0, 'o'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hm:n:o:", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'm':
            options.videosFile = optarg;
            break;
  
         case 'n':
            options.usersFile = optarg;
            break;
  
         case 'o':
            options.broadcastsFile = optarg;
            break;

         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }

   if (optind < argc) {
      printHelpText();
      exit(1);
   }
}

void setDefaultOptions()
{
   options.broadcastsFile = "broadcasts.csv";
   options.videosFile = "videos.csv";
   options.usersFile = "users.csv";
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
   mongo_init(&conn);

   mongo_replset_init(&conn, "gtRollFrame");
   mongo_replset_add_seed(&conn, "10.176.69.215", 27017); // gt-db-roll-frame-s0-a
   mongo_replset_add_seed(&conn, "10.176.69.216", 27017); // gt-db-roll-frame-s0-b

   int status_frame = mongo_replset_connect(&conn);
   printMongoConnectionErrorMessage(status_frame, conn);

   return MONGO_OK == mongo_cmd_authenticate(&conn, "gt-roll-frame", "gt_user", "GT/us3r!!!");
}

void getRollType(const string &rollString,
                 Roll *roll)
{
   bson_oid_t rollOid;
   bson_oid_from_string(&rollOid, rollString.c_str());

   bson query;
   bson_init(&query);
   bson_append_oid(&query, "_id", &rollOid);
   bson_finish(&query);
 
   bson fields;
   bson_init(&fields);
   bson_append_int(&fields, "_id", 0);
   bson_append_int(&fields, "h", 1); // genius
   bson_finish(&fields);

   bson out;
   bson_init(&out);
 
   if (mongo_find_one(&conn, "gt-roll-frame.rolls", &query, &fields, &out) == MONGO_OK) {
      bson_iterator iterator;
      bson_iterator_init(&iterator, &out);
      if (BSON_BOOL == bson_iterator_type(&iterator) &&
          bson_iterator_bool(&iterator)) {
         roll->type = GENIUS;
      } else {
         roll->type = OTHER;
      }
   }

   bson_destroy(&out);
   bson_destroy(&fields);
   bson_destroy(&query);
}

void processFrames(const string &broadcastsFileName,
                   const string &videosFileName,
                   const string &usersFileName)
{
   ofstream broadcastsFile, videosFile, usersFile;
   
   broadcastsFile.open(broadcastsFileName.c_str());
   videosFile.open(videosFileName.c_str());
   usersFile.open(usersFileName.c_str());

   bson fields;
   bson_init(&fields);
   bson_append_int(&fields, "_id", 0);
   bson_append_int(&fields, "a", 1);
   bson_append_int(&fields, "b", 1);
   bson_finish(&fields);

   mongo_cursor cursor;
   mongo_cursor_init(&cursor, &conn, "gt-roll-frame.frames");
   mongo_cursor_set_options(&cursor, MONGO_SLAVE_OK);
   mongo_cursor_set_fields(&cursor, &fields);
 
   while (MONGO_OK == mongo_cursor_next(&cursor)) {
      const bson *frame = mongo_cursor_bson(&cursor);
      bson_iterator iterator;
      bson_type type;
      char rollBuffer[128];
      char videoBuffer[128];
      unsigned int rollId, videoId;
      rollType rollType;

      type = bson_find(&iterator, frame, "a"); // roll_id
      if (BSON_OID != type) {
         continue;
      }
      bson_oid_t *roll = bson_iterator_oid(&iterator);
      bson_oid_to_string(roll, rollBuffer);
      string rollString(rollBuffer);

      type = bson_find(&iterator, frame, "b"); // video_id
      if (BSON_OID != type) {
         continue;
      }
      bson_oid_t *video = bson_iterator_oid(&iterator);
      bson_oid_to_string(video, videoBuffer);
      string videoString(videoBuffer);

      map<string, Roll>::iterator rollIter;
     
      if ((rollIter = rolls.find(rollString)) == rolls.end()) {
         Roll newRoll;
         newRoll.type = UNINITIALIZED;
         rollId = newRoll.id = ++uniqueRollsCount;
         getRollType(rollString, &newRoll);
         rollType = newRoll.type;
         rolls.insert(pair<string, Roll>(rollString, newRoll));
         usersFile << newRoll.id << "," << rollString << endl;
      } else {
         rollId = rollIter->second.id;
         rollType = rollIter->second.type;
      }
      
      map<string, unsigned int>::iterator videoIter;

      if ((videoIter = videoOids.find(videoString)) == videoOids.end()) {
         videoId = ++uniqueVideosCount;
         videoOids.insert(pair<string, unsigned int>(videoString, videoId));
         videosFile << videoId << "," << videoString << endl;
      } else {
         videoId = videoIter->second;
      }
     
      if (rollType == OTHER) {
         broadcastsFile << rollId << "," << videoId << endl; 
      } 
   }
  
   mongo_cursor_destroy(&cursor);
   bson_destroy(&fields);

   broadcastsFile.close();
   videosFile.close();
   usersFile.close();
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

   processFrames(options.broadcastsFile, options.videosFile, options.usersFile);

mongoCleanup:
   mongo_destroy(&conn);
   return status;
}
