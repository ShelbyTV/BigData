#ifndef __ITEM_BASED_FILTERING_H__
#define __ITEM_BASED_FILTERING_H__

#include <vector>
#include <string>
#include <fstream>

class ItemBasedFiltering
{

public:

   ItemBasedFiltering();

   bool parseInputFile(const std::string &inputCSV);
   void outputAllRecommendations(const unsigned int numThreads,
                                 const unsigned int numRecs,
                                 const std::string &outputFileName);

   void outputUserRecommendations(const unsigned int userID,
                                  const unsigned int numThreads,
                                  const unsigned int numRecs,
                                  const std::string &outputFileName);

   void outputItemRecommendations(const unsigned int itemID,
				  const unsigned int numThreads,
                                  const unsigned int numRecs,
                                  const std::string &outputFileName);

private:

   bool populateUserAndItemVectors(const std::string &inputCSV);
   void calculateTotalUsers();
   void sortItemToUsersVectorsWorkerThread(const unsigned int threadID,
                                           const unsigned int numThreads);

   void sortItemToUsersVectors(const unsigned int numThreads);
   void generateAndOutputAllRecommendations(const unsigned int numThreads,
                                            const unsigned int numRecs,
                                            const std::string &outputFile);
   void generateAllRecsWorkerThread(const unsigned int threadID, 
                                    const unsigned int numThreads,
                                    const unsigned int numRecs,
                                    const std::string &outputFileName);
   void generateRecsForItem(const unsigned int itemID,
                            const unsigned int numRecs,
                            std::ofstream &outputFile);

   unsigned int totalUsers;
   std::vector< std::vector<unsigned int> > itemsToUsers; 
   std::vector< std::vector<unsigned int> > usersToItems; 

};

#endif // __ITEM_BASED_FILTERING_H__
