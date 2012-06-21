#ifndef __K_MEDIOID_H__
#define __K_MEDIOID_H__

#include <vector>
#include <string>
#include <fstream>

class KMedioid
{

public:

   KMedioid();

   bool parseInputFile(const std::string &inputCSV);
   void outputAllClusters(const unsigned int numThreads,
                          const unsigned int clusters,
                          const std::string &outputFileName);

private:

   bool populateUserAndItemVectors(const std::string &inputCSV);
   void calculateTotalUsers();
   void sortItemToUsersVectorsWorkerThread(const unsigned int threadID,
                                           const unsigned int numThreads);

   void sortItemToUsersVectors(const unsigned int numThreads);

   unsigned int totalUsers;
   std::vector< std::vector<unsigned int> > itemsToUsers; 
   std::vector< std::vector<unsigned int> > usersToItems; 

   std::vector< std::vector<unsigned int> > clusters;
   std::vector< std::vector<unsigned int> > clusterVectors;
};

#endif // __K_MEDIOID_H__
