#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <mutex>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include "common/parallelize.h"
#include "common/commandOutput.h"

#include "lib/mongo-c-driver/src/mongo.h"

using namespace std;

static struct Options {
   string inputFile;
   string outputFile;
   string videoMapFile;
   string userMapFile;
   unsigned int filethreads;
   unsigned int particularUserID;
   unsigned int particularVideoID;
} options; 

void printHelpText()
{
   cout << "gtRecs usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -i --input       Specify input file (user,item CSV format) (default: input.csv)" << endl;
   cout << "   -m --videomap    Video map file containing id to site/id translation (default: videos.csv)" << endl;
   cout << "   -n --usermap     User map file containing id to network/id translation (default: users.csv)" << endl;
   cout << "   -o --output      Specify recommendations output file (default: output)" << endl;
   cout << "   -t --filethreads Number of threads to use for file operations while running (default: 4)" << endl;
   cout << "   -u --user        Output recommendations for a particular roll ID; cannot be combined with -v" << endl;
   cout << "   -v --video       Output recommendations for a particular video ID; cannot be combined with -u" << endl;
}

mongo conn;

typedef struct Video
{
   string id;
   string title;
} Video;

static mutex videosMutex;
static map<unsigned int, Video> videos;

static mutex videoRecsMultimapsMutex;
static map<unsigned int, multimap<double, unsigned int> > videoRecs;

string exec(char* cmd)
{
   char buffer[128];
   string result = "";
  
   START_COMMAND_OUTPUT_LOOP(cmd, buffer, 128) {
      result += buffer;
   } END_COMMAND_OUTPUT_LOOP

   return result;
}

void loadUserVideoShares(const unsigned int userID, const string &inputFileName)
{
   static const string shellCommandPart1 = "grep \"^";
   static const string shellCommandPart2 = ",\" ";

   char buffer[1024];
   ostringstream stringStream;
   stringStream << shellCommandPart1 << userID << shellCommandPart2 << inputFileName;
  
   cout << "Loading user video shares..." << endl;

   START_COMMAND_OUTPUT_LOOP((char *)stringStream.str().c_str(), buffer, 1024) {
      
      unsigned int user = 0, item = 0;

      if (2 != sscanf(buffer, "%u,%u", &user, &item)) {
         continue; // ignore bad lines
      }

      if (videoRecs.find(item) == videoRecs.end()) {
         multimap<double, unsigned int> newMultiMap;
         Video newVideo;

         videoRecs.insert(pair<unsigned int, multimap<double, unsigned int> >(item, newMultiMap));
         videos.insert(pair<unsigned int, Video>(item, newVideo));
      }

   } END_COMMAND_OUTPUT_LOOP

   cout << "User video shares loaded." << endl;
}

void insertVideoRecommendation(const unsigned int item,
                               const unsigned int rec,
                               const double val)
{
   map<unsigned int, multimap<double, unsigned int> >::iterator mapItem = videoRecs.find(item);

   if (mapItem == videoRecs.end()) {
      return;
   }

   {
      lock_guard<mutex> lock(videoRecsMultimapsMutex); // better to have one mutex per item, eventually
      mapItem->second.insert(pair<double, unsigned int>(val, rec));
   }

   {
      lock_guard<mutex> lock(videosMutex); 
      if (videos.find(rec) == videos.end()) {
         Video newVideo;
         videos.insert(pair<unsigned int, Video>(rec, newVideo));
      }
   }
}

void loadVideoRecommendationsWorkerThread(const string &recsFileName,
                                          const unsigned int threadID,
                                          const unsigned int numThreads)
{
   unsigned int count = 0;
   ifstream recsFile;
   string line;

   recsFile.open(recsFileName);
   while (recsFile.good()) {
      unsigned int item = 0, rec = 0;
      double val = 0;

      getline(recsFile, line);

      if ((++count) % numThreads != threadID) {
         continue;
      }
   
      if (3 != sscanf(line.c_str(), "%u %u %lf", &item, &rec, &val)) {
         continue;
      }
 
      insertVideoRecommendation(item, rec, val); 
   }

   recsFile.close();
}

void loadVideoRecommendationsWithGrep(const string &recsFileName)
{
   static const string shellCommandPart1 = "grep \"^";
   static const string shellCommandPart2 = " \" ";

   char buffer[1024];

   for (map<unsigned int, multimap<double, unsigned int> >::const_iterator iter = videoRecs.begin();
        iter != videoRecs.end();
        ++iter) {

      ostringstream stringStream;
      stringStream << shellCommandPart1 << iter->first << shellCommandPart2 << recsFileName;
  
      START_COMMAND_OUTPUT_LOOP((char *)stringStream.str().c_str(), buffer, 1024) {
         
         unsigned int item = 0, rec = 0;
         double val = 0;

         if (3 != sscanf(buffer, "%u %u %lf", &item, &rec, &val)) {
            continue;
         }
         
         insertVideoRecommendation(item, rec, val); 

      } END_COMMAND_OUTPUT_LOOP
   }
}

void loadVideoRecommendations(const unsigned int numThreads,
                              const string &recsFileName)
{
   cout << "Loading recommendations for videos..." << endl;
   if (videoRecs.size() == 1) {
      loadVideoRecommendationsWithGrep(recsFileName); 
   } else {
      PARALLELIZE(numThreads, loadVideoRecommendationsWorkerThread, recsFileName);
   }
   cout << "Recommendations for videos loaded." << endl;
}

void loadVideoIDsWorkerThread(const string &videoMapFileName,
                              const unsigned int threadID,
                              const unsigned int numThreads)
{
   ifstream videoMapFile;
   string line;
   unsigned int count = 0;

   videoMapFile.open(videoMapFileName);
   while (videoMapFile.good()) {
      char id[1024];
      unsigned int item;

      getline(videoMapFile, line);

      if ((++count) % numThreads != threadID) {
         continue;
      }
 
      if (2 != sscanf(line.c_str(), "%u,%1023s", &item, id)) {
         continue;
      }
      
      if (videos.find(item) == videos.end()) {
         continue;
      }

      videos.find(item)->second.id = id;
   }
}

void loadVideoIDs(const unsigned int numThreads,
                  const string &videoMapFileName)
{
   cout << "Loading video IDs..." << endl;
   PARALLELIZE(numThreads, loadVideoIDsWorkerThread, videoMapFileName);
   cout << "Video IDs loaded." << endl;
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

void loadVideoTitles()
{
   cout << "Loading video titles..." << endl;

   if (!connectToMongoAndAuthenticate()) {
      cout << "Could not connect to mongo..." << endl;

      mongo_destroy(&conn);
      return;
   }

   map<unsigned int, Video>::iterator iter = videos.begin();
   for (; iter != videos.end(); ++iter) {
      if (iter->second.title != "") {
         cout << "Video title already set..." << endl;
         continue;
      }

      bson_oid_t videoOid;
      bson_oid_from_string(&videoOid, iter->second.id.c_str());

      bson query;
      bson_init(&query);
      bson_append_oid(&query, "_id", &videoOid);
      bson_finish(&query);
   
      bson fields;
      bson_init(&fields);
      bson_append_int(&fields, "_id", 0);
      bson_append_int(&fields, "c", 1);
      bson_finish(&fields);
   
      bson out;
      bson_init(&out);
   
      if (mongo_find_one(&conn, "gt-video.videos", &query, &fields, &out) == MONGO_OK) {
         bson_iterator iterator;
         bson_iterator_init(&iterator, &out);
         iter->second.title = bson_iterator_string(&iterator);
      } else {
         cout << "Could not find video..." << endl;
      }

      bson_destroy(&out);
      bson_destroy(&fields);
      bson_destroy(&query);
   }

   cout << "Video titles loaded." << endl;

   mongo_destroy(&conn);
}

void printResults()
{
   cout << "************************************************************" << endl;
   cout << endl;

   for (map<unsigned int, multimap<double, unsigned int> >::const_iterator iter = videoRecs.begin();
        iter != videoRecs.end();
        ++iter) {

      cout.width(12); 
      cout << videos.find(iter->first)->second.id << ": " << videos.find(iter->first)->second.title.substr(0, 80) << endl;
      
      for (multimap<double, unsigned int>::const_reverse_iterator riter = iter->second.rbegin();
           riter != iter->second.rend();
           ++riter) {

         cout.width(10);
         cout << setiosflags(ios::fixed) << setprecision(3);
         cout << riter->first << " ";
         cout.width(12);
         cout << videos.find(riter->second)->second.id << ": ";
         cout << videos.find(riter->second)->second.title.substr(0, 80) << endl;
      }

      cout << endl;
   }
   cout << "************************************************************" << endl;
}

void setProcessPriority()
{
   int success = setpriority(PRIO_PROCESS, getpid(), 19);
   if (success == -1) {
      cerr << "Error setting process priority to 19." << endl;
   }
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",        no_argument,       0, 'h'},
         {"input",       required_argument, 0, 'i'},
         {"videomap",    required_argument, 0, 'm'},
         {"usermap",     required_argument, 0, 'n'},
         {"output",      required_argument, 0, 'o'},
         {"filethreads", required_argument, 0, 't'},
         {"user",        required_argument, 0, 'u'},
         {"video",       required_argument, 0, 'v'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hi:m:n:o:t:u:v:", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'i':
            options.inputFile = optarg;
            break;
  
         case 'm':
            options.videoMapFile = optarg;
            break;
  
         case 'n':
            options.userMapFile = optarg;
            break;
  
         case 'o':
            options.outputFile = optarg;
            break;

         case 't':
            options.filethreads = atoi(optarg);
            break;
 
         case 'u':
            options.particularUserID = atoi(optarg);
            break;

         case 'v':
            options.particularVideoID = atoi(optarg);
            break;
 
         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }

   if (optind < argc || options.filethreads == 0) {
      printHelpText();
      exit(1);
   }
}

void checkOptions()
{
   unsigned int count = 0;

   count += (options.particularUserID != numeric_limits<unsigned int>::max());
   count += (options.particularVideoID != numeric_limits<unsigned int>::max());

   if (count != 1) {
      printHelpText();
      exit(1);
   }
}

void setDefaultOptions()
{
   options.inputFile = "input.csv";
   options.outputFile = "output";
   options.videoMapFile = "videos.csv";
   options.userMapFile = "users.csv";
   options.filethreads = 4;
   options.particularUserID = numeric_limits<unsigned int>::max();
   options.particularVideoID = numeric_limits<unsigned int>::max();
}

int main(int argc, char **argv)
{
   setProcessPriority();

   setDefaultOptions();
   parseUserOptions(argc, argv);
   checkOptions();

   if (options.particularUserID != numeric_limits<unsigned int>::max()) {

      loadUserVideoShares(options.particularUserID, options.inputFile);

   } else {

      multimap<double, unsigned int> newMultiMap;
      Video newVideo;
      unsigned int videoID = 0;

      videoID = options.particularVideoID;

      videoRecs.insert(pair<unsigned int, multimap<double, unsigned int> >(videoID, newMultiMap));
      videos.insert(pair<unsigned int, Video>(videoID, newVideo));
   }

   loadVideoRecommendations(options.filethreads, options.outputFile);
   loadVideoIDs(options.filethreads, options.videoMapFile);
   loadVideoTitles();

   printResults();

   return 0;
}
