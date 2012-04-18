#include "itemBasedFiltering.h"
#include "topNSimilarItems.h"
#include "logLikelihood.h"

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <sstream>
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
      return false;
   }

   while (input.good()) {
      string line;   
      unsigned int user = 0, item = 0;
      
      getline(input, line);

      if (2 != sscanf(line.c_str(), "%u,%u", &user, &item)) {
         continue; // ignore bad lines
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

void ItemBasedFiltering::sortItemToUsersVectors(const unsigned int numThreads)
{
   // XXX multithreading needed

   for (unsigned int i = 0; i < itemsToUsers.size(); i++) {
      if (itemsToUsers[i].size() != 0) {
         sort(itemsToUsers[i].begin(), itemsToUsers[i].end());
      }
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

void ItemBasedFiltering::generateAndOutputRecommendedItems(unsigned int mod, unsigned int which)
{

   ostringstream stringStream;
   stringStream << "part" << which;
   
   ofstream outputFile;
   outputFile.open(stringStream.str().c_str());

   for (unsigned int i = 0; i < 20000 /* itemsToUsers.size() */; i++) {

      if (i % mod != which) {
         continue;
      } 

      TopNSimilarItems topNSimilarItems(20);
    
      set<unsigned int> potentialSimilarItems; 
      
      // loop over users for this item, then over those users, to find potential similar items
      for (unsigned int j = 0; j < itemsToUsers[i].size(); j++) {

         unsigned int userWhoSharedThisItem = itemsToUsers[i][j];
         
         for (unsigned int k = 0; k < usersToItems[userWhoSharedThisItem].size(); k++) {
 
            if (usersToItems[userWhoSharedThisItem][k] == i) {
               continue;
            }

            potentialSimilarItems.insert(usersToItems[userWhoSharedThisItem][k]);
         }
      }

      for (set<unsigned int>::const_iterator iter = potentialSimilarItems.begin();
           iter != potentialSimilarItems.end();
           ++iter) {

	 topNSimilarItems.add(LogLikelihood::ratio(itemsToUsers[i], itemsToUsers[*iter], totalUsers), *iter);
      }

      topNSimilarItems.print(i, outputFile);
   }

   outputFile.close();
}

void ItemBasedFiltering::outputRecommendations(const unsigned int numThreads,
                                               const unsigned int numRecs,
                                               const string &outputFile)
{
   sortItemToUsersVectors(numThreads);
   
   thread t0(&ItemBasedFiltering::generateAndOutputRecommendedItems, this, 4, 0);
   thread t1(&ItemBasedFiltering::generateAndOutputRecommendedItems, this, 4, 1);
   thread t2(&ItemBasedFiltering::generateAndOutputRecommendedItems, this, 4, 2);
   thread t3(&ItemBasedFiltering::generateAndOutputRecommendedItems, this, 4, 3);
   

   t0.join();
   t1.join();
   t2.join();
   t3.join();
}
