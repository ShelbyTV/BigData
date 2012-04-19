#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

using namespace std;

static vector<unsigned int> itemCount; 
static vector< vector<unsigned int> > usersToItems; 

static struct Options {
   string broadcastsFile;
   string outputFile;
   unsigned int itemLimit;
   unsigned int userLimit;
} options;

void printHelpText()
{
   cout << "mongoDataPrep usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
   cout << "   -i --input       Input name for broadcasts file (default: broadcasts.csv)" << endl;
   cout << "   -m --itemLimit   Number of users an item must have to pass pruning (default: 5)" << endl;
   cout << "   -o --output      Output name for pruned broadcasts file (default: input.csv)" << endl;
   cout << "   -u --userLimit   Number of items a user must have to pass pruning (default: 5)" << endl;
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",        no_argument,       0, 'h'},
         {"input",       required_argument, 0, 'i'},
         {"itemLimit",   required_argument, 0, 'm'},
         {"output",      required_argument, 0, 'o'},
         {"userLimit",   required_argument, 0, 'u'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "hi:m:o:u:", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'i':
            options.broadcastsFile = optarg;
            break;
 
         case 'm':
            options.itemLimit = atoi(optarg);
            break;
  
         case 'o':
            options.outputFile = optarg;
            break;

         case 'u':
            options.userLimit = atoi(optarg);
            break;
 
         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }

   if (optind < argc) {
      printHelpText();
      exit(1);
   }
}

void setDefaultOptions()
{
   options.broadcastsFile = "broadcasts.csv";
   options.outputFile = "input.csv";
   options.userLimit = 5;
   options.itemLimit = 5;
}

void processInput(const string &broadcastsFileName)
{
   ifstream input(broadcastsFileName.c_str());
   if (!input.is_open()) {
      cerr << "Failed to open input file." << endl;
      exit(1);
   }

   while (input.good()) {
      string line;   
      unsigned int user = 0, item = 0;
      
      getline(input, line);

      if (2 != sscanf(line.c_str(), "%u,%u", &user, &item)) {
         continue; // ignore bad lines
      }

      if (item >= itemCount.size()) {
         itemCount.resize(2 * item);
      }

      if (user >= usersToItems.size()) {
         usersToItems.resize(2 * user);
      }

      if (usersToItems[user].size() > 0 &&
          find(usersToItems[user].begin(), usersToItems[user].end(), item) != usersToItems[user].end())
      {
         continue; // duplicate user,item combination
      }

      itemCount[item]++;
      usersToItems[user].push_back(item);
   }
   
   input.close();
}

void outputPruned(const unsigned int itemLimit, 
                  const unsigned int userLimit, 
                  const string &outputFileName)
{
   ofstream output(outputFileName.c_str());
   if (!output.is_open()) {
      cerr << "Failed to open output file." << endl;
      exit(1);
   }

   for (unsigned int i = 0; i < usersToItems.size(); i++) {
      if (usersToItems[i].size() < userLimit) {
         continue;
      }

      for (unsigned int j = 0; j < usersToItems[i].size(); j++) {
         if (itemCount[usersToItems[i][j]] < itemLimit) {
            continue;
         }

         output << i << "," << usersToItems[i][j] << endl;
      }
   }

   output.close(); 
}

int main(int argc, char **argv)
{
   setDefaultOptions();
   parseUserOptions(argc, argv);

   processInput(options.broadcastsFile);
   outputPruned(options.itemLimit, options.userLimit, options.outputFile);

   return 0;
}
