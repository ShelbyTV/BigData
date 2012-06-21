#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "common/parallelize.h"

#include "kMedioid.h"
#include "topNSimilarItems.h"
#include "logLikelihood.h"

using namespace std;

KMedioid::KMedioid()
{
  totalUsers = 0;
}

void KMedioid::outputAllClusters(const unsigned int numThreads,
                                 const unsigned int clusters,
                                 const std::string &outputFileName)
{
   sortItemToUsersVectors(numThreads);

   // initial setup
   // pick clusters # of videos as initial cluster vectors
   TopNSimilarItems potentialInitialClusterVectors(clusters * 10);
   for (unsigned int i = 0; i < 100000; i++) {
   //    set<unsigned int> relatedItems;
   //    for (unsigned int j = 0; j < itemsToUsers[i].size(); j++) {
   //       unsigned int user = itemsToUsers[i][j];
   //       for (unsigned int k = 0; k < usersToItems[user].size(); k++) {
   //          relatedItems.insert(usersToItems[user][k]);
   //       }
   //    }
   //    potentialInitialClusterVectors.add(relatedItems.size(), i);
       potentialInitialClusterVectors.add(itemsToUsers[i].size(), i);
   }
   
   potentialInitialClusterVectors.print(0, cout);                


   // go through cluster vector users->videos and compute top LLR against all cluster vectors
   // add video to top cluster list
   
   // for each cluster, create new representative vector

}

bool KMedioid::populateUserAndItemVectors(const string &inputCSV)
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

void KMedioid::calculateTotalUsers()
{
   for (unsigned int i = 0; i < usersToItems.size(); i++) {
      if (usersToItems[i].size() != 0) {
         totalUsers++;
      }
   }
}

void KMedioid::sortItemToUsersVectorsWorkerThread(const unsigned int threadID,
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

void KMedioid::sortItemToUsersVectors(const unsigned int numThreads)
{
   PARALLELIZE(numThreads, &KMedioid::sortItemToUsersVectorsWorkerThread, this);
}

bool KMedioid::parseInputFile(const string &inputCSV)
{
   if (!populateUserAndItemVectors(inputCSV)) {
      return false;
   }

   calculateTotalUsers();
 
   return true;
}

// void KMedioid::generateRecsForItem(const unsigned int itemID,
//                                              const unsigned int numRecs,
//                                              ofstream &outputFile)
// {
//    TopNSimilarItems topNSimilarItems(numRecs);
//    set<unsigned int> potentials; 
// 
//    // loop over users for this item, then over those users, to find potential similar items
//    for (unsigned int j = 0; j < itemsToUsers[itemID].size(); j++) {
//        
//       unsigned int user = itemsToUsers[itemID][j];
//       
//       for (unsigned int k = 0; k < usersToItems[user].size(); k++) {
//          if (usersToItems[user][k] == itemID) {
//             continue;
//          }
//          potentials.insert(usersToItems[user][k]);
//       }
//    }
// 
//    for (set<unsigned int>::const_iterator iter = potentials.begin(); 
//         iter != potentials.end(); 
//         ++iter) {
// 
//       double similarity = LogLikelihood::ratio(itemsToUsers[itemID], 
//                                                itemsToUsers[*iter], 
//                                                totalUsers);
//       topNSimilarItems.add(similarity, *iter);
//    }
// 
//    topNSimilarItems.print(itemID, outputFile);
// }

// void KMedioid::generateAllRecsWorkerThread(const unsigned int numRecs,
//                                                      const string &outputFileName,
//                                                      const unsigned int threadID,
//                                                      const unsigned int numThreads)
// {
//    ostringstream stringStream;
//    stringStream << outputFileName << threadID;
//    
//    ofstream outputFile;
//    outputFile.open(stringStream.str().c_str());
// 
//    for (unsigned int i = 0; i < itemsToUsers.size(); i++) {
//       
//       if (numThreads > 1 && i % numThreads != threadID) {
//          continue;
//       }
// 
//      generateRecsForItem(i, numRecs, outputFile);
//    } 
//             
//    outputFile.close();
// }
// 
// void KMedioid::generateAndOutputAllRecommendations(const unsigned int numThreads,
//                                                              const unsigned int numRecs,
//                                                              const string &outputFile)
// {
//    PARALLELIZE(numThreads, &KMedioid::generateAllRecsWorkerThread, this, numRecs, outputFile);
// }

// void KMedioid::outputAllRecommendations(const unsigned int numThreads,
//                                                   const unsigned int numRecs,
//                                                   const string &outputFileName)
// {
//    sortItemToUsersVectors(numThreads);
//    generateAndOutputAllRecommendations(numThreads, numRecs, outputFileName);
// }
// 
// void KMedioid::outputUserRecommendations(const unsigned int userID,
//                                                    const unsigned int numThreads,
//                                                    const unsigned int numRecs,
//                                                    const std::string &outputFileName)
// {
//    ofstream outputFile;
//    
//    sortItemToUsersVectors(numThreads);
// 
//    outputFile.open(outputFileName.c_str());
// 
//    for (unsigned int i = 0; i < usersToItems[userID].size(); i++) {
//       generateRecsForItem(usersToItems[userID][i], numRecs, outputFile);
//    }
// 
//    outputFile.close();
// }
// 
// void KMedioid::outputItemRecommendations(const unsigned int itemID,
//                                                    const unsigned int numThreads,
//                                                    const unsigned int numRecs,
//                                                    const string &outputFileName)
// {
//    ofstream outputFile;
//    
//    sortItemToUsersVectors(numThreads);
// 
//    outputFile.open(outputFileName.c_str());
// 
//    generateRecsForItem(itemID, numRecs, outputFile);
// 
//    outputFile.close();
// }
