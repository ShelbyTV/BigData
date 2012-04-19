#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

using namespace std;

static struct Options {
   string broadcastsFile;
   string videosFile;
   string usersFile;
} options; 

void printHelpText()
{
   cout << "mongoDataPrep usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -m --videomap    Output name for video map file containing id to site/id translation (default: videos.csv)" << endl;
   cout << "   -n --usermap     Output name for user map file containing id to network/id translation (default: users.csv)" << endl;
   cout << "   -o --output      Output name for broadcasts file (default: broadcasts.csv)" << endl;
}

static unsigned int uniqueVideosCount = 0;
static map<string, map<string, unsigned int> > videos;

static unsigned int uniqueUsersCount = 0;
static map<string, map<string, unsigned int> > users;

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

unsigned int findOrInsertEntity(const string &outerKey, 
                                const string &innerKey,
                                map<string, map<string, unsigned int> > &entityMap,
                                unsigned int *entityCounter,
                                ofstream &entityFile)
{
   map<string, map<string, unsigned int> >::iterator iter1;
   map<string, unsigned int>::iterator iter2;

   if ((iter1 = entityMap.find(outerKey)) != entityMap.end()) {
      if ((iter2 = iter1->second.find(innerKey)) != iter1->second.end()) {
         return iter2->second;
      } else {
         iter1->second.insert(pair<string, unsigned int>(innerKey, ++(*entityCounter)));

         entityFile << (*entityCounter) << "," << outerKey << "," << innerKey << endl; 

         return (*entityCounter);
      }
   } else {
      map<string, unsigned int> newMap;
      newMap.insert(pair<string, unsigned int>(innerKey, ++(*entityCounter)));
      entityMap.insert(pair<string, map<string, unsigned int> >(outerKey, newMap));

      entityFile << (*entityCounter) << "," << outerKey << "," << innerKey << endl; 

      return (*entityCounter);
   }

   assert(false); // should never reach this point
   return 0; 
}

void processInput(const string &broadcastsFileName,
                  const string &videosFileName,
                  const string &usersFileName)
{
   string line;
   ofstream broadcastsFile, videosFile, usersFile;
   
   broadcastsFile.open(broadcastsFileName.c_str());
   videosFile.open(videosFileName.c_str());
   usersFile.open(usersFileName.c_str());

   while (cin.good()) {

      string network, userName, userImage, site, videoID;
      unsigned int video = 0, user = 0;

      char networkBuffer[1024];   // A
      char userNameBuffer[1024];  // x
      char userImageBuffer[1024]; // v
      char siteBuffer[1024];      // r
      char videoIDBuffer[1024];   // s

      getline(cin, line);

      if (5 != sscanf(line.c_str(),
          "\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",\"%[^\"]\"",
          networkBuffer,
          userNameBuffer,
          userImageBuffer,
          siteBuffer,
          videoIDBuffer))
      {
         cerr << "Couldn't parse CSV line: " << line << endl;
         continue;
      } 

      network = networkBuffer;
      userName = userNameBuffer;
      userImage = userImageBuffer;
      site = siteBuffer;
      videoID  = videoIDBuffer;

      video = findOrInsertEntity(site, videoID, videos, &uniqueVideosCount, videosFile);

      if (network == "facebook") {
         string userID;
         char userIDBuffer[1024];

         if (1 != sscanf(userImage.c_str(), "http://graph.facebook.com/%[^/]/picture" , userIDBuffer)) {
            cerr << "Couldn't parse Facebook userID from userImage URL: " << userImage << endl;
            continue;
         }

         userID = userIDBuffer; 
         user = findOrInsertEntity(network, userID, users, &uniqueUsersCount, usersFile);
      } else {
         user = findOrInsertEntity(network, userName, users, &uniqueUsersCount, usersFile);
      } 

      broadcastsFile << user << "," << video << endl; 
   }

   broadcastsFile.close();
   videosFile.close();
   usersFile.close();
}

int main(int argc, char **argv)
{
   setDefaultOptions();
   parseUserOptions(argc, argv);

   processInput(options.broadcastsFile, options.videosFile, options.usersFile);

   return 0;
}
