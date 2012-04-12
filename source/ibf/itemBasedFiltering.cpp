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
      if (i % numThreads != threadID) {
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

void ItemBasedFiltering::generateRecsWorkerThread(const unsigned int threadID, 
                                                  const unsigned int numThreads,
                                                  const unsigned int numRecs,
                                                  const string &outputFileName)
{
   ostringstream stringStream;
   stringStream << outputFileName << threadID;
   
   ofstream outputFile;
   outputFile.open(stringStream.str().c_str());

   for (unsigned int i = 0; i < itemsToUsers.size(); i++) {
      
      TopNSimilarItems topNSimilarItems(numRecs);
      set<unsigned int> potentials; 

      if (i % numThreads != threadID) {
         continue;
      } 
            
      // loop over users for this item, then over those users, to find potential similar items
      for (unsigned int j = 0; j < itemsToUsers[i].size(); j++) {
          
         unsigned int user = itemsToUsers[i][j];
         
         for (unsigned int k = 0; k < usersToItems[user].size(); k++) {
            if (usersToItems[user][k] == i) {
               continue;
            }
            potentials.insert(usersToItems[user][k]);
         }
      }

      for (set<unsigned int>::const_iterator iter = potentials.begin(); 
           iter != potentials.end(); 
           ++iter) {

         double similarity = LogLikelihood::ratio(itemsToUsers[i], 
                                                  itemsToUsers[*iter], 
                                                  totalUsers);
	 topNSimilarItems.add(similarity, *iter);
      }

      topNSimilarItems.print(i, outputFile);
   }

   outputFile.close();
}

void ItemBasedFiltering::generateAndOutputRecommendations(const unsigned int numThreads,
                                                          const unsigned int numRecs,
                                                          const string &outputFile)
{
   vector<thread> threads; 
  
   for (unsigned int i = 0; i < numThreads; i++) {

      threads.push_back(thread(&ItemBasedFiltering::generateRecsWorkerThread, 
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

void ItemBasedFiltering::outputRecommendations(const unsigned int numThreads,
                                               const unsigned int numRecs,
                                               const string &outputFile)
{
   sortItemToUsersVectors(numThreads);
   generateAndOutputRecommendations(numThreads, numRecs, outputFile); 
}
