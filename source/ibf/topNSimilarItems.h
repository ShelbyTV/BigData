#ifndef __TOP_N_SIMILAR_ITEMS_H__
#define __TOP_N_SIMILAR_ITEMS_H__

#include <map>
#include <fstream>

class TopNSimilarItems
{

public:

   TopNSimilarItems(const unsigned int maxItems);

   void add(const double similarity, const unsigned int similarItem);
   void print(const unsigned int originalItem, std::ofstream &outputFile);

private:

   unsigned int maxItems;
   std::multimap<double, unsigned int> similarItems;

};

#endif // __TOP_N_SIMILAR_ITEMS_H__
