#include "itemBasedFiltering.h"
#include "topNSimilarItems.h"
#include "logLikelihood.h"

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

ItemBasedFiltering::ItemBasedFiltering()
{
  totalUsers = 0;
}

bool ItemBasedFiltering::populateUserAndItemVectors(const string &inputCSV)
{
   ifstream input(inputCSV.c_str());
   if (!input.is_open()) {
      cerr << "Failed to open input file." << endl;
      return false;
   }

   while (input.good()) {
      string line;   
      unsigned int user = 0, item = 0;
      
      getline(input, line);

      if (2 != sscanf(line.c_str(), "%u,%u", &user, &item)) {
         continue; // ignore bad lines
      }

      if (item >= itemsToUsers.size()) {
         itemsToUsers.resize(2 * item);
      }

      if (user >= usersToItems.size()) {
         usersToItems.resize(2 * user);
      }

      itemsToUsers[item].push_back(user);
      usersToItems[user].push_back(item);
   }
   
   input.close();

   return true; 
}

void ItemBasedFiltering::calculateTotalUsers()
{
   for (unsigned int i = 0; i < usersToItems.size(); i++) {
      if (usersToItems[i].size() != 0) {
         totalUsers++;
      }
   }
}

void ItemBasedFiltering::sortItemToUsersVectorsWorkerThread(const unsigned int threadID,
                                                            const unsigned int numThreads)
{
   for (unsigned int i = 0; i < itemsToUsers.size(); i++) {
      if (numThreads > 1 && i % numThreads != threadID) {
         continue;
      } else if (itemsToUsers[i].size() != 0) {
         sort(itemsToUsers[i].begin(), itemsToUsers[i].end());
      }
   }
}

void ItemBasedFiltering::sortItemToUsersVectors(const unsigned int numThreads)
{
   vector<thread> threads; 
  
   for (unsigned int i = 0; i < numThreads; i++) {
      threads.push_back(thread(&ItemBasedFiltering::sortItemToUsersVectorsWorkerThread, this, i, numThreads));
   }

   for (unsigned int i = 0; i < numThreads; i++) {
      threads[i].join();
   }
}

bool ItemBasedFiltering::parseInputFile(const string &inputCSV)
{
   if (!populateUserAndItemVectors(inputCSV)) {
      return false;
   }

   calculateTotalUsers();
 
   return true;
}

void ItemBasedFiltering::generateRecsForItem(const unsigned int itemID,
                                             const unsigned int numRecs,
                                             ofstream &outputFile)
{
   TopNSimilarItems topNSimilarItems(numRecs);
   set<unsigned int> potentials; 

   // loop over users for this item, then over those users, to find potential similar items
   for (unsigned int j = 0; j < itemsToUsers[itemID].size(); j++) {
       
      unsigned int user = itemsToUsers[itemID][j];
      
      for (unsigned int k = 0; k < usersToItems[user].size(); k++) {
         if (usersToItems[user][k] == itemID) {
            continue;
         }
         potentials.insert(usersToItems[user][k]);
      }
   }

   for (set<unsigned int>::const_iterator iter = potentials.begin(); 
        iter != potentials.end(); 
        ++iter) {

      double similarity = LogLikelihood::ratio(itemsToUsers[itemID], 
                                               itemsToUsers[*iter], 
                                               totalUsers);
      topNSimilarItems.add(similarity, *iter);
   }

   topNSimilarItems.print(itemID, outputFile);
}

void ItemBasedFiltering::generateAllRecsWorkerThread(const unsigned int threadID, 
                                                     const unsigned int numThreads,
                                                     const unsigned int numRecs,
                                                     const string &outputFileName)
{
   ostringstream stringStream;
   stringStream << outputFileName << threadID;
   
   ofstream outputFile;
   outputFile.open(stringStream.str().c_str());

   for (unsigned int i = 0; i < itemsToUsers.size(); i++) {
      
      if (numThreads > 1 && i % numThreads != threadID) {
         continue;
      }

      generateRecsForItem(i, numRecs, outputFile);
   } 
            
   outputFile.close();
}

void ItemBasedFiltering::generateAndOutputAllRecommendations(const unsigned int numThreads,
                                                             const unsigned int numRecs,
                                                             const string &outputFile)
{
   vector<thread> threads; 
  
   for (unsigned int i = 0; i < numThreads; i++) {

      threads.push_back(thread(&ItemBasedFiltering::generateAllRecsWorkerThread, 
                               this, 
                               i, 
                               numThreads, 
                               numRecs, 
                               outputFile));

   }

   for (unsigned int i = 0; i < numThreads; i++) {
      threads[i].join();
   }
}

void ItemBasedFiltering::outputAllRecommendations(const unsigned int numThreads,
                                                  const unsigned int numRecs,
                                                  const string &outputFileName)
{
   sortItemToUsersVectors(numThreads);
   generateAndOutputAllRecommendations(numThreads, numRecs, outputFileName);
}

void ItemBasedFiltering::outputUserRecommendations(const unsigned int userID,
                                                   const unsigned int numThreads,
                                                   const unsigned int numRecs,
                                                   const std::string &outputFileName)
{
   ofstream outputFile;
   
   sortItemToUsersVectors(numThreads);

   outputFile.open(outputFileName.c_str());

   for (unsigned int i = 0; i < usersToItems[userID].size(); i++) {
      generateRecsForItem(usersToItems[userID][i], numRecs, outputFile);
   }

   outputFile.close();
}

void ItemBasedFiltering::outputItemRecommendations(const unsigned int itemID,
                                                   const unsigned int numThreads,
                                                   const unsigned int numRecs,
                                                   const string &outputFileName)
{
   ofstream outputFile;
   
   sortItemToUsersVectors(numThreads);

   outputFile.open(outputFileName.c_str());

   generateRecsForItem(itemID, numRecs, outputFile);

   outputFile.close();
}
