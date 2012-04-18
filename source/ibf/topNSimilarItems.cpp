#include "topNSimilarItems.h"
#include <iomanip>

using namespace std;

TopNSimilarItems::TopNSimilarItems(unsigned int maxItems)
{
   this->maxItems = maxItems;
} 

void TopNSimilarItems::add(double similarity, unsigned int similarItem)
{
   if (similarity < 0.000001) {
      return;
   }

   this->similarItems.insert(pair<double, unsigned int>(similarity, similarItem));

   if (this->similarItems.size() > this->maxItems) {
      this->similarItems.erase(this->similarItems.begin());
   }
}

void TopNSimilarItems::print(unsigned int originalItem, ofstream &outputFile)
{
   typedef multimap<double, unsigned int>::const_reverse_iterator 
      SimilarItemsRevIter;

   for (SimilarItemsRevIter iter = this->similarItems.rbegin();
        iter != this->similarItems.rend();
        ++iter) 
   {
      outputFile << originalItem << " ";
      outputFile << iter->second << " ";
      outputFile << setiosflags(ios::fixed) << setprecision(3);
      outputFile << iter->first << endl;
   }
}

