#ifndef __ITEM_BASED_FILTERING_H__
#define __ITEM_BASED_FILTERING_H__

#include <vector>
#include <string>

class ItemBasedFiltering
{

public:

   ItemBasedFiltering();

   bool parseInputFile(const std::string &inputCSV);
   void outputRecommendations(const unsigned int numThreads,
                              const unsigned int numRecs,
                              const std::string &outputFile);

private:

   bool populateUserAndItemVectors(const std::string &inputCSV);
   void calculateTotalUsers();
   void sortItemToUsersVectors(const unsigned int numThreads);
   void generateAndOutputRecommendedItems(const unsigned int mod, 
                                          const unsigned int which);

   unsigned int totalUsers;
   std::vector< std::vector<unsigned int> > itemsToUsers; 
   std::vector< std::vector<unsigned int> > usersToItems; 

};

#endif // __ITEM_BASED_FILTERING_H__
