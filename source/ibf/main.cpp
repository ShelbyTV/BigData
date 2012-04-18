#include "itemBasedFiltering.h"

#include <string>

using namespace std;

int main()
{
   ItemBasedFiltering ibf;
   bool ok = true;

   // parse commandline options   
   string inputCSV;
   unsigned int numThreads = 0;
   unsigned int numRecs = 0;
   string outputFile;

   ok = ibf.parseInputFile(inputCSV);

   if (!ok) {
      return -1;
   }

   ibf.outputRecommendations(numThreads,
                             numRecs,
                             outputFile);

   return 0; 
}
