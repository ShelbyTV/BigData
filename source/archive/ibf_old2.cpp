#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>

using namespace std;

class SimilarItems
{
public:

   SimilarItems(int maxItems)
   {
      this->maxItems = maxItems;
   } 

   void addItem(double similarity, unsigned long item)
   {
      if (similarity < 0.000001) {
         return;
      }

      this->items.insert(pair<double, unsigned long>(similarity, item));
      if (this->items.size() > this->maxItems) {
         this->items.erase(this->items.begin());
      }
   }

   void print(unsigned long originalItem)
   {
      for (multimap<double, unsigned long>::const_reverse_iterator multimapIter = this->items.rbegin();
           multimapIter != this->items.rend();
           ++multimapIter) {
         cout << originalItem << " " << multimapIter->second << " " << setiosflags(ios::fixed) << setprecision(3) << multimapIter->first << endl;
      }

      flush(cout); 
   }

private:

   int maxItems;
   multimap<double, unsigned long> items;
};

map<unsigned long, set<unsigned long> > itemsToUsers; 

double tanimoto(set<unsigned long>one, set<unsigned long>two)
{
   set<unsigned long>setIntersection;
   set<unsigned long>setUnion;

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

   while (input.good()) {
      
      string line;   
      unsigned long user = 0, item = 0;
      
      getline(input, line);

      if (2 != sscanf(line.c_str(), "%lu,%lu", &user, &item)) {
         continue;
      }
         
      map<unsigned long, set<unsigned long> >::iterator itemIterator;
      if ((itemIterator = itemsToUsers.find(item)) != itemsToUsers.end()) {
         itemIterator->second.insert(user);
      } else {
         set <unsigned long> newSet;
         newSet.insert(user);
         itemsToUsers.insert(pair<unsigned long, set<unsigned long> >(item, newSet));
      }
   }

   input.close();
}

void generateAndOutputRecommendedItems()
{
   for (map<unsigned long, set<unsigned long> >::const_iterator itemOneIter = itemsToUsers.begin();
        itemOneIter != itemsToUsers.end();
        ++itemOneIter) {

      SimilarItems similarItems(20);
    
      set<unsigned long> itemOneUserSet = itemOneIter->second;
      
      for (map<unsigned long, set<unsigned long> >::const_iterator itemTwoIter = itemsToUsers.begin();
           itemTwoIter != itemsToUsers.end(); 
           ++itemTwoIter) {
         
         if (itemTwoIter->first == itemOneIter->first) {
            continue;
         }

         set<unsigned long> itemTwoUserSet = itemTwoIter->second;
         
         similarItems.addItem(tanimoto(itemOneUserSet, itemTwoUserSet), itemTwoIter->first);
      }

      similarItems.print(itemOneIter->first);
   }
}

int main()
{
   readInputFileIntoMap();
   generateAndOutputRecommendedItems();
 
   return 0; 
}
