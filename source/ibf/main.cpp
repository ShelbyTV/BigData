#include <iostream>
#include <string>
#include <limits>

#include <getopt.h>
#include <stdlib.h>

#include "itemBasedFiltering.h"

using namespace std;

static struct Options {
   string inputFile;
   string outputFile;
   unsigned int threads;
   unsigned int recs;
   unsigned int particularUser;
   unsigned int particularItem;
} options; 

void printHelpText()
{
   cout << "ibf usage:" << endl;
   cout << "   -h --help       Print this help message" << endl;
   cout << "   -i --input      Specify input file (user,item CSV format)" << endl;
   cout << "   -o --output     Specify output file" << endl;
   cout << "   -t --threads    Number of threads to use while running" << endl;
   cout << "   -r --recs       Number of recommendations to provider per item" << endl;
   cout << "   -u --user       Output recommendations for a particular user; cannot be combined with -m" << endl;
   cout << "   -m --item       Output recommendations for a particular item; cannot be combined with -u" << endl;
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",    no_argument,       0, 'h'},
         {"input",   required_argument, 0, 'i'},
         {"output",  required_argument, 0, 'o'},
         {"threads", required_argument, 0, 't'},
         {"recs",    required_argument, 0, 'r'},
         {"user",    required_argument, 0, 'u'},
         {"item",    required_argument, 0, 'm'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hi:o:t:r:u:m:", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'i':
            options.inputFile = optarg;
            break;
   
         case 'o':
            options.outputFile = optarg;
            break;
   
         case 't':
            options.threads = atoi(optarg);
            break;
   
         case 'r':
            options.recs = atoi(optarg);
            break;
    
         case 'u':
            options.particularUser = atoi(optarg);
            break;

         case 'm':
            options.particularItem = atoi(optarg);
            break;
        
         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }

   if (optind < argc || options.recs == 0 || options.threads == 0) {
      printHelpText();
      exit(1);
   }
}

void checkOptions()
{
   if (options.particularUser != numeric_limits<unsigned int>::max() &&
       options.particularItem != numeric_limits<unsigned int>::max()) {
      printHelpText();
      exit(1);
   }
}

void performIBF()
{
   ItemBasedFiltering ibf;
  
   if (!ibf.parseInputFile(options.inputFile)) {
      exit(1);
   }   

   if (options.particularUser != numeric_limits<unsigned int>::max()) {
      ibf.outputUserRecommendations(options.particularUser,
                                    options.threads,
				    options.recs,
				    options.outputFile);
   } else if (options.particularItem != numeric_limits<unsigned int>::max()) {
      ibf.outputItemRecommendations(options.particularItem,
                                    options.threads,
                                    options.recs,
                                    options.outputFile); 
   } else {
      ibf.outputAllRecommendations(options.threads,
                                   options.recs,
                                   options.outputFile);
   }
}

int main(int argc, char **argv)
{
   // default options
   options.inputFile = "input.csv";
   options.outputFile = "output";
   options.threads = 1;
   options.recs = 20;
   options.particularUser = numeric_limits<unsigned int>::max();
   options.particularItem = numeric_limits<unsigned int>::max();

   parseUserOptions(argc, argv);

   checkOptions();

   performIBF();
   
   return 0;
}
