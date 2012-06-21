#include <iostream>
#include <string>
#include <limits>

#include <getopt.h>
#include <stdlib.h>

#include "kMedioid.h"

using namespace std;

static struct Options {
   string inputFile;
   string outputFile;
   unsigned int threads;
   unsigned int clusters;
} options; 

void printHelpText()
{
   cout << "kMedioid usage:" << endl;
   cout << "   -h --help       Print this help message" << endl;
   cout << "   -i --input      Specify input file (user,item CSV format)" << endl;
   cout << "   -o --output     Specify output file" << endl;
   cout << "   -t --threads    Number of threads to use while running" << endl;
   cout << "   -c --clusters   Number of clusters to form" << endl;
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",      no_argument,       0, 'h'},
         {"input",     required_argument, 0, 'i'},
         {"output",    required_argument, 0, 'o'},
         {"threads",   required_argument, 0, 't'},
         {"clusters",  required_argument, 0, 'c'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hi:o:t:c:", long_options, &option_index);
   
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
   
         case 'c':
            options.clusters = atoi(optarg);
            break;
    
         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }

   if (optind < argc || options.clusters == 0 || options.threads == 0) {
      printHelpText();
      exit(1);
   }
}

void checkOptions()
{
}

void performKMedioid()
{
   KMedioid kMedioid;
  
   if (!kMedioid.parseInputFile(options.inputFile)) {
      exit(1);
   }   

   kMedioid.outputAllClusters(options.threads,
                              options.clusters,
                              options.outputFile);
}

int main(int argc, char **argv)
{
   // default options
   options.inputFile = "input.csv";
   options.outputFile = "output";
   options.threads = 1;
   options.clusters = 1000;

   parseUserOptions(argc, argv);
   checkOptions();

   performKMedioid();
   
   return 0;
}
