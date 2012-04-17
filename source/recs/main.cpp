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

using namespace std;

#define START_COMMAND_OUTPUT_LOOP(command, buffer, size)         \
{                                                                \
   FILE *commandOutputLoopPipe = popen(command, "r");            \
   if (!commandOutputLoopPipe) {                                 \
      exit(1);                                                   \
   }                                                             \
                                                                 \
   while (!feof(commandOutputLoopPipe)) {                        \
      if (fgets(buffer, size, commandOutputLoopPipe) == NULL) {  \
         break;                                                  \
      }

#define END_COMMAND_OUTPUT_LOOP                                  \
   }                                                             \
   pclose(commandOutputLoopPipe);                                \
}

#define PARALLELIZE(numThreads, function, ...)                   \
{                                                                \
   vector<thread> parallelizeThreads;                            \
   for (unsigned int parallelizeCounter = 0;                     \
        parallelizeCounter < numThreads;                         \
        parallelizeCounter++) {                                  \
                                                                 \
      parallelizeThreads.push_back(thread(function,              \
                                          parallelizeCounter,    \
                                          numThreads ,           \
                                          ##__VA_ARGS__));       \
   }                                                             \
                                                                 \
   for (unsigned int parallelizeCounter = 0;                     \
        parallelizeCounter < numThreads;                         \
        parallelizeCounter++) {                                  \
                                                                 \
      parallelizeThreads[parallelizeCounter].join();             \
   }                                                             \
}

static struct Options {
   string inputFile;
   string outputFile;
   string videoMapFile;
   string userMapFile;
   string ytvideo;
   string twitter;
   unsigned int ytthreads;
   unsigned int filethreads;
   unsigned int particularUserID;
   unsigned int particularVideoID;
} options; 

void printHelpText()
{
   cout << "recs usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -i --input       Specify input file (user,item CSV format) (default: item.csv)" << endl;
   cout << "   -m --videomap    Video map file containing id to site/id translation (default: videoMapFile)" << endl;
   cout << "   -n --usermap     User map file containing id to network/id translation (default: userMapFile)" << endl;
   cout << "   -o --output      Specify recommendations output file (default: output)" << endl;
   cout << "   -t --filethreads Number of threads to use for file operations while running (default: 4)" << endl;
   cout << "   -u --user        Output recommendations for a particular user ID; cannot be combined with -v, -w, -z" << endl;
   cout << "   -v --video       Output recommendations for a particular video ID; cannot be combined with -u, -w, -z" << endl;
   cout << "   -w --twitter     Output recommendations for a particular Twitter user ID; cannot be combined with -v, -u, -z" << endl;
   cout << "   -y --ytthreads   Number of threads to use for YouTube titles while running (default: 32)" << endl;
   cout << "   -z --ytvideo     Output recommendations for a particular YouTube video ID; cannot be combined with -v, -u, -w" << endl;
}

typedef struct Video
{
   string site;
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

string getYouTubeTitle(const string &youTubeID) 
{
   static const string shellCommandPart1 = "curl -s \"http://www.youtube.com/oembed?url=http://ww.youtube.com/watch?v=";
   static const string shellCommandPart2 = "\" | sed 's#.*\"title\": \"\\([^\"]*\\)\".*#\\1#g'";

   ostringstream stringStream;
   stringStream << shellCommandPart1 << youTubeID << shellCommandPart2;

   return exec((char *)stringStream.str().c_str());
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

void loadVideoRecommendationsWorkerThread(const unsigned int threadID,
                                          const unsigned int numThreads,
                                          const string &recsFileName)
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

void loadVideoIDsWorkerThread(const unsigned int threadID,
                              const unsigned int numThreads,
                              const string &videoMapFileName)
{
   ifstream videoMapFile;
   string line;
   unsigned int count = 0;

   videoMapFile.open(videoMapFileName);
   while (videoMapFile.good()) {
      char site[128];
      char id[128];
      unsigned int item;

      getline(videoMapFile, line);

      if ((++count) % numThreads != threadID) {
         continue;
      }
 
      if (3 != sscanf(line.c_str(), "%u => %127s %127s", &item, site, id)) {
         continue;
      }
      
      if (videos.find(item) == videos.end()) {
         continue;
      }

      videos.find(item)->second.site = site;
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

void loadYouTubeVideoTitlesWorkerThread(const unsigned int threadID, 
                                        const unsigned int numThreads)
{
   unsigned int count = 0;
   map<unsigned int, Video>::iterator iter = videos.begin();
   for (; iter != videos.end(); ++iter, ++count) {

      if (count % numThreads != threadID) {
         continue;
      }
    
      if (iter->second.site != "youtube" || iter->second.title != "") {
         continue;
      }

      iter->second.title = getYouTubeTitle(iter->second.id);
   }
}

void loadYouTubeVideoTitles(const unsigned int numThreads)
{
   cout << "Loading YouTube video titles..." << endl;
   PARALLELIZE(numThreads, loadYouTubeVideoTitlesWorkerThread);
   cout << "YouTube video titles loaded." << endl;
}

unsigned int loadTwitterUserID(const string &twitterUsername, 
                               const string &userMapFileName)
{
   static const string shellCommandPart1 = "grep -m1 \"twitter ";
   static const string shellCommandPart2 = "$\" ";
   static const string shellCommandPart3 = " | sed 's#^\\([0-9]*\\) .*#\\1#g'";

   ostringstream stringStream;
   stringStream << shellCommandPart1 << twitterUsername << shellCommandPart2 << userMapFileName << shellCommandPart3;
  
   cout << "Loading twitter userID..." << endl;
   string result = exec((char *)stringStream.str().c_str());
   unsigned int userID = atoi(result.c_str());

   if (userID <= 0) {
      cerr << "Error loading twitter userID." << endl;
      exit(1);
   }

   cout << "Twitter userID loaded." << endl;

   return userID;
}

unsigned int loadYouTubeVideoID(const string &youtubeVideo, 
                                const string &videoMapFileName)
{
   static const string shellCommandPart1 = "grep -m1 \"youtube ";
   static const string shellCommandPart2 = "$\" ";
   static const string shellCommandPart3 = " | sed 's#^\\([0-9]*\\) .*#\\1#g'";

   ostringstream stringStream;
   stringStream << shellCommandPart1 << youtubeVideo << shellCommandPart2 << videoMapFileName << shellCommandPart3;
  
   cout << "Loading YouTube videoID..." << endl;
   string result = exec((char *)stringStream.str().c_str());
   unsigned int videoID = atoi(result.c_str());

   if (videoID <= 0) {
      cerr << "Error loading YouTube videoID." << endl;
      exit(1);
   }

   cout << "YouTube videoID loaded." << endl;

   return videoID;
}

void printResults()
{
   cout << "************************************************************" << endl;
   cout << endl;

   for (map<unsigned int, multimap<double, unsigned int> >::const_iterator iter = videoRecs.begin();
        iter != videoRecs.end();
        ++iter) {

      if (videos.find(iter->first)->second.site != "youtube") {
         continue;
      }
     
      cout.width(12); 
      cout << videos.find(iter->first)->second.id << ": " << videos.find(iter->first)->second.title << endl;
      
      for (multimap<double, unsigned int>::const_reverse_iterator riter = iter->second.rbegin();
           riter != iter->second.rend();
           ++riter) {

         if (videos.find(riter->second)->second.site != "youtube") {
            continue;
         }

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
         {"twitter",     required_argument, 0, 'w'},
         {"ytthreads",   required_argument, 0, 'y'},
         {"ytvideo",     required_argument, 0, 'z'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hi:m:n:o:t:u:v:w:y:z:", long_options, &option_index);
   
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
 
         case 'w':
            options.twitter = optarg;
            break;
 
         case 'y':
            options.ytthreads = atoi(optarg);
            break;
   
         case 'z':
            options.ytvideo = optarg;
            break;

         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }

   if (optind < argc || options.ytthreads == 0 || options.filethreads == 0) {
      printHelpText();
      exit(1);
   }
}

void checkOptions()
{
   unsigned int count = 0;

   count += (options.particularUserID != numeric_limits<unsigned int>::max());
   count += (options.particularVideoID != numeric_limits<unsigned int>::max());
   count += (options.ytvideo != "");
   count += (options.twitter != "");

   if (count != 1) {
      printHelpText();
      exit(1);
   }
}

void setDefaultOptions()
{
   options.inputFile = "input.csv";
   options.outputFile = "output";
   options.videoMapFile = "videoMapFile";
   options.userMapFile = "userMapFile";
   options.ytthreads = 32;
   options.filethreads = 4;
   options.particularUserID = numeric_limits<unsigned int>::max();
   options.particularVideoID = numeric_limits<unsigned int>::max();
   options.twitter = "";
   options.ytvideo = ""; 
}

int main(int argc, char **argv)
{
   setProcessPriority();

   setDefaultOptions();
   parseUserOptions(argc, argv);
   checkOptions();

   if (options.twitter != "" || options.particularUserID != numeric_limits<unsigned int>::max()) {

      unsigned int userID = 0;

      if (options.twitter != "") {
         userID = loadTwitterUserID(options.twitter, options.userMapFile);
      } else {
         userID = options.particularUserID;
      }

      loadUserVideoShares(userID, options.inputFile);

   } else {

      multimap<double, unsigned int> newMultiMap;
      Video newVideo;
      unsigned int videoID = 0;

      if (options.ytvideo != "") {
         videoID = loadYouTubeVideoID(options.ytvideo, options.videoMapFile);
      } else {
         videoID = options.particularVideoID;
      }

      videoRecs.insert(pair<unsigned int, multimap<double, unsigned int> >(videoID, newMultiMap));
      videos.insert(pair<unsigned int, Video>(videoID, newVideo));
   }

   loadVideoRecommendations(options.filethreads, options.outputFile);
   loadVideoIDs(options.filethreads, options.videoMapFile);
   loadYouTubeVideoTitles(options.ytthreads);

   printResults();

   return 0;
}
