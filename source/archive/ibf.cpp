#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <thread>
#include <sstream>

using namespace std;

class SimilarItems
{
public:

   SimilarItems(int maxItems)
   {
      this->maxItems = maxItems;
   } 

   void addItem(double similarity, unsigned int item)
   {
      if (similarity < 0.000001) {
         return;
      }

      this->items.insert(pair<double, unsigned int>(similarity, item));
      if (this->items.size() > this->maxItems) {
         this->items.erase(this->items.begin());
      }
   }

   void print(unsigned int originalItem, ofstream &outputFile)
   {
      for (multimap<double, unsigned int>::const_reverse_iterator multimapIter = this->items.rbegin();
           multimapIter != this->items.rend();
           ++multimapIter) {
         outputFile << originalItem << " " << multimapIter->second << " " << setiosflags(ios::fixed) << setprecision(3) << multimapIter->first << endl;
      }
   }

private:

   int maxItems;
   multimap<double, unsigned int> items;
};

unsigned int totalUsers;
vector< vector<unsigned int> > itemsToUsers; 
vector< vector<unsigned int> > usersToItems; 

double xLogX(unsigned int x)
{
   return x == 0 ? 0.0 : (double)x * log((double)x);
}

double entropy(unsigned int a, unsigned int b)
{
   return xLogX(a + b) - xLogX(a) - xLogX(b);
}

double entropy(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
   return xLogX(a + b + c + d) - xLogX(a) - xLogX(b) - xLogX(c) - xLogX(d);
}

double loglikelihood(const vector<unsigned int> &one, const vector<unsigned int> &two)
{
   unsigned int numUsersWhoSharedOne = one.size();
   unsigned int numUsersWhoSharedTwo = two.size();

   set<unsigned int> intersection;
   set_intersection(one.begin(), one.end(), two.begin(), two.end(), inserter(intersection, intersection.end()));

   unsigned int k11 = intersection.size(); // both together
   unsigned int k12 = numUsersWhoSharedTwo - k11; // two, not one
   unsigned int k21 = numUsersWhoSharedOne - k11; // one, not two
   unsigned int k22 = totalUsers - (k12 + k21 + k11); // otherwise

   double rowEntropy = entropy(k11, k12) + entropy(k21, k22);
   double columnEntropy = entropy(k11, k21) + entropy(k12, k22);
   double matrixEntropy = entropy(k11, k12, k21, k22);
   if (rowEntropy + columnEntropy > matrixEntropy) {
      // round off error
      return 0.0;
   }
   
   return 2.0 * (matrixEntropy - rowEntropy - columnEntropy);
}

void readInputFileIntoMap()
{
   ifstream input("input.csv");
 
   if (!input.is_open()) {
      return;
   }

   itemsToUsers.resize(16000000);
   usersToItems.resize(7000000);

   while (input.good()) {
      
      string line;   
      unsigned int user = 0, item = 0;
      
      getline(input, line);

      if (2 != sscanf(line.c_str(), "%u,%u", &user, &item)) {
         continue;
      }

      itemsToUsers[item].push_back(user);
      usersToItems[user].push_back(item);
   }
   
   totalUsers = 0;
   for (int i = 0; i < usersToItems.size(); i++) {
      if (usersToItems[i].size() != 0) {
         totalUsers++;
      }
   }

   for (int i = 0; i < itemsToUsers.size(); i++) {
      if (itemsToUsers[i].size() != 0) {
         sort(itemsToUsers[i].begin(), itemsToUsers[i].end());
      }
   }

   input.close();
}

void generateAndOutputRecommendedItems()
{
   ostringstream stringStream;
   stringStream << "output";
   
   ofstream outputFile;
   outputFile.open(stringStream.str());

   for (unsigned int i = 13325464; i < 13325465/* itemsToUsers.size() */; i++) {

      SimilarItems similarItems(20);
    
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

	 similarItems.addItem(loglikelihood(itemsToUsers[i], itemsToUsers[*iter]), *iter);
      }

      similarItems.print(i, outputFile);
   }

   outputFile.close();
}

int main()
{
   readInputFileIntoMap();
   generateAndOutputRecommendedItems();
 
   return 0; 
}
