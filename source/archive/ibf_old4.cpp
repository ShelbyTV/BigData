#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <functional>

using namespace std;

class SimilarItems
{
public:

   SimilarItems(int maxItems)
   {
      this->maxItems = maxItems;
      cutoff = 0.0;
   } 

   void addItem(double similarity, unsigned int item)
   {
      if (similarity < 0.000001) {
         return;
      }

      this->items.insert(pair<double, unsigned int>(similarity, item));
      if (this->items.size() > this->maxItems) {
         this->items.erase(this->items.begin());
         this->cutoff = this->items.begin()->first;
      }
   }

   void print(unsigned int originalItem)
   {
      for (multimap<double, unsigned int>::const_reverse_iterator multimapIter = this->items.rbegin();
           multimapIter != this->items.rend();
           ++multimapIter) {
         cout << originalItem << " " << multimapIter->second << " " << setiosflags(ios::fixed) << setprecision(3) << multimapIter->first << endl;
      }
   }

   double getCutoff()
   {
      return this->cutoff;
   }

private:

   int maxItems;
   multimap<double, unsigned int> items;
   double cutoff;
};

vector< vector<unsigned int> > itemsToUsers; 
vector< vector<unsigned int> > usersToItems; 
vector< size_t > itemsToMinHashes;
multimap< size_t, unsigned int > minHashesToItems;

double tanimoto(set<unsigned int>one, set<unsigned int>two)
{
   set<unsigned int>setIntersection;
   set<unsigned int>setUnion;

   set_intersection(one.begin(), one.end(), two.begin(), two.end(), inserter(setIntersection, setIntersection.end()));
   set_union(one.begin(), one.end(), two.begin(), two.end(), inserter(setUnion, setUnion.end()));

   return ((double)setIntersection.size() / (double)setUnion.size());
}

void readInputFileIntoMap()
{
   ifstream input("input.csv");
 
   if (!input.is_open()) {
      return;
   }

   itemsToUsers.resize(16000000);
   itemsToMinHashes.resize(16000000);
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

   input.close();
}

void generateAndOutputRecommendedItems()
{
   // loop over all items
   for (unsigned int i = 0; i < 16000000; i++) {

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

      set<unsigned int>itemOneUserSet(itemsToUsers[i].begin(), itemsToUsers[i].end());

      for (set<unsigned int>::const_iterator iter = potentialSimilarItems.begin();
           iter != potentialSimilarItems.end();
           ++iter) {

         // biggest possible intersection / smallest possible union
         if (similarItems.getCutoff() > (double)min(itemOneUserSet.size(), itemsToUsers[*iter].size()) / (double)max(itemOneUserSet.size(), itemsToUsers[*iter].size())) {
            continue;
         } else {
	    set<unsigned int>itemTwoUserSet(itemsToUsers[*iter].begin(), itemsToUsers[*iter].end());
	    similarItems.addItem(tanimoto(itemOneUserSet, itemTwoUserSet), *iter);
	 }
      }

      similarItems.print(i);
   }
}

size_t generateItemMinHash(unsigned int item)
{
   hash<unsigned int> userHash;
   size_t minHash = std::numeric_limits<std::size_t>::max();
   size_t minHash2 = std::numeric_limits<std::size_t>::max();
   size_t minHash3 = std::numeric_limits<std::size_t>::max();

   for (unsigned int u = 0; u < itemsToUsers[item].size(); u++) {
         size_t temp = userHash(itemsToUsers[item][u]);
         if (temp <= minHash) {
            minHash3 = minHash2;
            minHash2 = minHash;
            minHash = temp;
         } else if (temp <= minHash2) {
            minHash3 = minHash2;
            minHash2 = temp;
         } else if (temp <= minHash3) {
            minHash3 = temp;
         }
   }
   
   return (minHash ^ minHash2 ^ minHash3); 
}

void generateMinHashesAndOutputRecommendedItems()
{
   for (unsigned int i = 0; i < 16000000; i++) {
      if (itemsToUsers[i].size() < 5) {
         continue;
      }

      size_t minHash = generateItemMinHash(i);
      itemsToMinHashes[i] = minHash;
      minHashesToItems.insert(pair<size_t, unsigned int>(minHash, i)); 
   }
   
   for (unsigned int i = 0; i < 16000000; i++) {

       typedef multimap<size_t, unsigned int>::iterator iterator;
       pair<iterator, iterator> p = minHashesToItems.equal_range(itemsToMinHashes[i]);

       for (iterator iter = p.first; iter != p.second; ++iter) {
         cout << i << " " << iter->second << endl;
      }
   }
}

int main()
{
   readInputFileIntoMap();
   //generateAndOutputRecommendedItems();
   generateMinHashesAndOutputRecommendedItems();
   
   return 0; 
}
