#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <limits>
#include <mutex>
#include <cstdatomic>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include "common/commandOutput.h"
#include "common/parallelize.h"

using namespace std;

static string executableDirectoryPath;
static mutex usersMutex;
static map<string, unsigned int> users;
static atomic<unsigned int> totalUsers(0);

static struct Options {
   string outputFile;
   string videoMapFile;
   string userMapFile;
} options; 

void printHelpText()
{
   cout << "youtubeDataPrep usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -m --videomap    Video map file containing id to site/id translation (default: videos.csv)" << endl;
   cout << "   -o --output      Specify recommendations output file (default: youtubeCommentsInput.csv)" << endl;
}

void determineExecutableDirectoryPath()
{
   char buffer[1024];
   ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
   if (len != -1) {
      buffer[len] = '\0';
      string bufferString(buffer);
      executableDirectoryPath = bufferString.substr(0, bufferString.rfind('/'));
   } else {
      exit(1);
   }
}

void processVideosWorkerThread(const string &videoMapFileName,
                               const string &userMapFileName,
                               const string &outputFileName,
                               const unsigned int threadID,
                               const unsigned int numThreads)
{
   ostringstream outputFileThreadName;
   outputFileThreadName << outputFileName << threadID;

   ostringstream userMapFileThreadName;
   userMapFileThreadName << userMapFileName << threadID;
   
   ofstream userMapFile;
   userMapFile.open(userMapFileThreadName.str().c_str());
   
   ofstream outputFile;
   outputFile.open(outputFileThreadName.str().c_str());

   ifstream videoMapFile;
   string line;

   videoMapFile.open(videoMapFileName);

   unsigned int count = 0;

   while (videoMapFile.good()) {
      char site[1024];
      char id[1024];
      unsigned int item;

      getline(videoMapFile, line);

      count++;

      if (numThreads > 1 && count % numThreads != threadID) {
         continue;
      }

      if (3 != sscanf(line.c_str(), "%u,%[^,],%1023s", &item, site, id)) {
         continue;
      }

      string siteString(site);
      string idString(id);
      
      if (siteString != "youtube") {
         continue;
      }

      char buffer[1024];
      ostringstream stringStream;
      stringStream << executableDirectoryPath << "/youtubeCommentersDownload.py" << " " << idString
                   << " | sort -u";
 
      START_COMMAND_OUTPUT_LOOP((char *)stringStream.str().c_str(), buffer, 1024) {
         string bufferString(buffer);
         string youtubeUser = bufferString.substr(0, bufferString.rfind('\n'));
         map<string, unsigned int>::const_iterator iter;
         map<string, unsigned int>::const_iterator end;
         unsigned int userNum = 0; 
 
         {
            lock_guard<mutex> lock(usersMutex);
            iter = users.find(youtubeUser);
            end = users.end();
            if (iter == end) {
               // new user
               userNum = totalUsers.fetch_add(1);
               pair<string, unsigned int> newPair(youtubeUser, userNum);
               users.insert(newPair);
            } else {
               userNum = iter->second;
            }
         }

         if (iter == end) { 
            userMapFile << userNum << "," << youtubeUser << endl;
         }

         outputFile << item << "," << userNum << endl;

      } END_COMMAND_OUTPUT_LOOP
   }

   videoMapFile.close();
}

void processVideos(const unsigned int numThreads,
                   const string &videoMapFileName,
                   const string &userMapFileName, 
                   const string &outputFileName)
{
   PARALLELIZE(numThreads, &processVideosWorkerThread, videoMapFileName, userMapFileName, outputFileName);
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
   options.outputFile = "youtubeCommentsInput.csv";
   options.videoMapFile = "videos.csv";
   options.userMapFile = "youtubeUserMap.csv";
}

void setProcessPriority()
{
   int success = setpriority(PRIO_PROCESS, getpid(), 19);
   if (success == -1) {
      cerr << "Error setting process priority to 19." << endl;
   }
}

int main(int argc, char **argv)
{
   setProcessPriority();

   setDefaultOptions();
   parseUserOptions(argc, argv);

   determineExecutableDirectoryPath();
   
   processVideos(10, options.videoMapFile, options.userMapFile, options.outputFile);

   return 0;
}
