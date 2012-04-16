#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <mutex>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

string exec(char* cmd)
{
   char buffer[128];
   string result = "";
   
   FILE* pipe = popen(cmd, "r");

   if (!pipe) {
      return "ERROR";
   }

   while (!feof(pipe)) {
      if (fgets(buffer, 128, pipe) != NULL) {
         result += buffer;
      }
   }
   pclose(pipe);

   return result;
}

typedef struct Video
{
   string site;
   string id;
   string title;
} Video;

static mutex videosMutex;
static map<unsigned int, Video> videos;

static map<unsigned int, multimap<double, unsigned int> > sharedVideoRecs;

string getYouTubeTitle(const string &youTubeID) 
{
   ostringstream stringStream;

   static string shellCommandPart1 = "curl -s \"http://www.youtube.com/oembed?url=http://ww.youtube.com/watch?v=";
   static string shellCommandPart2 = "\" | sed 's#.*\"title\": \"\\([^\"]*\\)\".*#\\1#g'";

   stringStream << shellCommandPart1 << youTubeID << shellCommandPart2;

   string result = exec((char *)stringStream.str().c_str());

   return result;
}

void loadUserVideoShares(const unsigned int userID)
{
   ostringstream stringStream;
   char buffer[1024];
 
   static string shellCommandPart1 = "grep \"^";
   static string shellCommandPart2 = ",\" ";

   stringStream << shellCommandPart1 << userID << shellCommandPart2 << "input.csv";
  
   cout << "Loading user video shares..." << endl;

   FILE* pipe = popen((char *)stringStream.str().c_str(), "r");

   if (!pipe) {
      exit(1);
   }

   while (!feof(pipe)) {
      unsigned int user = 0, item = 0;

      if (fgets(buffer, 1024, pipe) == NULL) {
         break;
      }

      if (2 != sscanf(buffer, "%u,%u", &user, &item)) {
         continue; // ignore bad lines
      }

      if (sharedVideoRecs.find(item) == sharedVideoRecs.end()) {
         multimap<double, unsigned int> newMultiMap;
         Video newVideo;

         sharedVideoRecs.insert(pair<unsigned int, multimap<double, unsigned int> >(item, newMultiMap));
         videos.insert(pair<unsigned int, Video>(item, newVideo));
      }
   }
   pclose(pipe);

   cout << "User video shares loaded." << endl;
}

void loadVideoRecommendationsWorkerThread(const unsigned int threadID,
                                          const unsigned int numThreads)
{
   char buffer[1024];
   unsigned int count = 0;
 
   static string shellCommandPart1 = "grep -m 20 \"^";
   static string shellCommandPart2 = " \" ";

   for (map<unsigned int, multimap<double, unsigned int> >::iterator iter = sharedVideoRecs.begin();
        iter != sharedVideoRecs.end();
        ++iter) {

      ++count;

      if (count % numThreads != threadID) {
         continue;
      }

      ostringstream stringStream;
      stringStream << shellCommandPart1 << iter->first << shellCommandPart2 << "output";
   
      FILE* pipe = popen((char *)stringStream.str().c_str(), "r");
   
      if (!pipe) {
         exit(1);
      }
   
      while (!feof(pipe)) {
         unsigned int item = 0, rec = 0;
         double val = 0;
   
         if (fgets(buffer, 1024, pipe) == NULL) {
            break;
         }

         if (3 != sscanf(buffer, "%u %u %lf", &item, &rec, &val)) {
            continue;
         }
   
         // assuming for now no duplicate rec items
         iter->second.insert(pair<double, unsigned int>(val, rec));
  
         {
            lock_guard<mutex> lock(videosMutex); 
            if (videos.find(rec) == videos.end()) {
               Video newVideo;
               videos.insert(pair<unsigned int, Video>(rec, newVideo));
            }
         }
      }

      pclose(pipe);
   }
}

void loadVideoRecommendations(const unsigned int numThreads)
{
   vector<thread> threads; 
   cout << "Loading recommendations for videos..." << endl;
  
   for (unsigned int i = 0; i < numThreads; i++) {

      threads.push_back(thread(loadVideoRecommendationsWorkerThread,
                               i, 
                               numThreads));

   }

   for (unsigned int i = 0; i < numThreads; i++) {
      threads[i].join();
   }
  
   cout << "Recommendations for videos loaded." << endl;
}

void loadVideoIDs()
{
   ifstream videoMapFile;
   string line;

   cout << "Loading video IDs..." << endl;
   videoMapFile.open("videoMapFile");
   while (videoMapFile.good()) {
      char site[128];
      char id[128];
      unsigned int item;

      getline(videoMapFile, line);

      if (3 != sscanf(line.c_str(), "%u => %127s %127s", &item, site, id)) {
         continue;
      }
      
      if (videos.find(item) == videos.end()) {
         continue;
      }

      videos.find(item)->second.site = site;
      videos.find(item)->second.id = id;
   }
   cout << "Video IDs loaded." << endl;
}

void loadYouTubeVideoTitlesWorkerThread(const unsigned int threadID, 
                                        const unsigned int numThreads)
{
   unsigned int count = 0;

   for (map<unsigned int, Video>::iterator iter = videos.begin();
        iter != videos.end();
        ++iter) {

      count++;

      if (count % numThreads != threadID) {
         continue;
      }
    
      if (iter->second.site != "youtube" && iter->second.title == "") {
         continue;
      }

      iter->second.title = getYouTubeTitle(iter->second.id);
   }
}

void loadYouTubeVideoTitles(const unsigned int numThreads)
{
   vector<thread> threads; 
   cout << "Loading YouTube video titles..." << endl;
  
   for (unsigned int i = 0; i < numThreads; i++) {

      threads.push_back(thread(loadYouTubeVideoTitlesWorkerThread,
                               i, 
                               numThreads));

   }

   for (unsigned int i = 0; i < numThreads; i++) {
      threads[i].join();
   }
  
   cout << "YouTube video titles loaded." << endl;
}

void printResults(const unsigned int userID)
{
   cout << "************************************************************" << endl;
   cout << "User id: " << userID << endl;
   cout << sharedVideoRecs.size() << " video shares" << endl;
   cout << endl;

   for (map<unsigned int, multimap<double, unsigned int> >::const_iterator iter = sharedVideoRecs.begin();
        iter != sharedVideoRecs.end();
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
         cout << videos.find(riter->second)->second.id << ": " << videos.find(riter->second)->second.title << endl;
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

int main()
{
   const unsigned int userID = 345210; // twitter markerrj

   setProcessPriority();

   loadUserVideoShares(userID);
   loadVideoRecommendations(4);
   loadVideoIDs();
   loadYouTubeVideoTitles(32);

   printResults(userID);

   return 0;
}

