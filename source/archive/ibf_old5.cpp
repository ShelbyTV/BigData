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
#include <sstream>

#define NUM_HASHES 11

using namespace std;

class HashVector {

public:

   HashVector() {}
   
   vector<unsigned int>getHashVector(unsigned int id) 
   {
      vector<unsigned int> hashVector;
      hashVector.resize(NUM_HASHES);
   
      ostringstream stringStream;
      stringStream << (id + 10000); // make sure id is long enough?
      string idStr = stringStream.str(); 
 
      hashVector[0] = RSHash(idStr);
      hashVector[1] = JSHash(idStr);
      hashVector[2] = PJWHash(idStr);
      hashVector[3] = ELFHash(idStr);
      hashVector[4] = BKDRHash(idStr);
      hashVector[5] = SDBMHash(idStr);
      hashVector[6] = DJBHash(idStr);
      hashVector[7] = DEKHash(idStr);
      hashVector[8] = BPHash(idStr);
      hashVector[9] = FNVHash(idStr);
      hashVector[10] = APHash(idStr);  

      return hashVector;
   }

private:

   unsigned int RSHash(const std::string& str)
   {
      unsigned int b    = 378551;
      unsigned int a    = 63689;
      unsigned int hash = 0;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = hash * a + str[i];
         a    = a * b;
      }
   
      return hash;
   }
   
   unsigned int JSHash(const std::string& str)
   {
      unsigned int hash = 1315423911;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash ^= ((hash << 5) + str[i] + (hash >> 2));
      }
   
      return hash;
   }
   
   unsigned int PJWHash(const std::string& str)
   {
      unsigned int BitsInUnsignedInt = (unsigned int)(sizeof(unsigned int) * 8);
      unsigned int ThreeQuarters     = (unsigned int)((BitsInUnsignedInt  * 3) / 4);
      unsigned int OneEighth         = (unsigned int)(BitsInUnsignedInt / 8);
      unsigned int HighBits          = (unsigned int)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
      unsigned int hash              = 0;
      unsigned int test              = 0;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = (hash << OneEighth) + str[i];
   
         if((test = hash & HighBits)  != 0)
         {
            hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
         }
      }
   
      return hash;
   }
   
   unsigned int ELFHash(const std::string& str)
   {
      unsigned int hash = 0;
      unsigned int x    = 0;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = (hash << 4) + str[i];
         if((x = hash & 0xF0000000L) != 0)
         {
            hash ^= (x >> 24);
         }
         hash &= ~x;
      }
   
      return hash;
   }
   
   unsigned int BKDRHash(const std::string& str)
   {
      unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
      unsigned int hash = 0;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = (hash * seed) + str[i];
      }
   
      return hash;
   }
   
   unsigned int SDBMHash(const std::string& str)
   {
      unsigned int hash = 0;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = str[i] + (hash << 6) + (hash << 16) - hash;
      }
   
      return hash;
   }
   
   unsigned int DJBHash(const std::string& str)
   {
      unsigned int hash = 5381;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = ((hash << 5) + hash) + str[i];
      }
   
      return hash;
   }
   
   unsigned int DEKHash(const std::string& str)
   {
      unsigned int hash = static_cast<unsigned int>(str.length());
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = ((hash << 5) ^ (hash >> 27)) ^ str[i];
      }
   
      return hash;
   }
   
   unsigned int BPHash(const std::string& str)
   {
      unsigned int hash = 0;
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash = hash << 7 ^ str[i];
      }
   
      return hash;
   }
   
   unsigned int FNVHash(const std::string& str)
   {
      const unsigned int fnv_prime = 0x811C9DC5;
      unsigned int hash = 0;
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash *= fnv_prime;
         hash ^= str[i];
      }
   
      return hash;
   }
   
   unsigned int APHash(const std::string& str)
   {
      unsigned int hash = 0xAAAAAAAA;
   
      for(std::size_t i = 0; i < str.length(); i++)
      {
         hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ str[i] * (hash >> 3)) :
                                  (~((hash << 11) + (str[i] ^ (hash >> 5))));
      }
   
      return hash;
   }
};

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

   void print(unsigned int originalItem)
   {
      for (multimap<double, unsigned int>::const_reverse_iterator multimapIter = this->items.rbegin();
           multimapIter != this->items.rend();
           ++multimapIter) {
         cout << originalItem << " " << multimapIter->second << " " << setiosflags(ios::fixed) << setprecision(3) << multimapIter->first << endl;
      }
   }

private:

   int maxItems;
   multimap<double, unsigned int> items;
};

vector< vector<unsigned int> > itemsToUsers; 
vector< vector<unsigned int> > itemsToMinHashes;
//vector< multimap< unsigned int, unsigned int > > minHashesToItems;

void readInputFileIntoMap()
{
   ifstream input("input.csv");
 
   if (!input.is_open()) {
      return;
   }

   itemsToUsers.resize(16000000);
   itemsToMinHashes.resize(16000000);
   //minHashesToItems.resize(NUM_HASHES);

   while (input.good()) {
      
      string line;   
      unsigned int user = 0, item = 0;
      
      getline(input, line);

      if (2 != sscanf(line.c_str(), "%u,%u", &user, &item)) {
         continue;
      }

      itemsToUsers[item].push_back(user);
   }

   input.close();
}

double hashVectorSimilarity(unsigned int itemOne, unsigned int itemTwo)
{
   vector<unsigned int> itemOneHashVector = itemsToMinHashes[itemOne];
   vector<unsigned int> itemTwoHashVector = itemsToMinHashes[itemTwo];

   int matches = 0;

   for (int i = 0; i < NUM_HASHES; i++) {
      if (itemOneHashVector[i] == itemTwoHashVector[i]) {
         matches++;
      }
   }

   return (double)matches / (double)NUM_HASHES;
}

void generateAndOutputRecommendedItems()
{
   // loop over all items
   for (unsigned int i = 0; i < itemsToUsers.size(); i++) {
      if (itemsToUsers[i].size() <= 0) {
         continue;
      }

      SimilarItems similarItems(20);
    
//      set<unsigned int> potentialSimilarItems; 
//      
//      typedef multimap<unsigned int, unsigned int>::iterator iterator;
//      for (int j = 0; j < NUM_HASHES; j++) {
//         
//         pair<iterator, iterator> p = minHashesToItems[j].equal_range(itemsToMinHashes[i][j]);
//      
//         for (iterator iter = p.first; iter != p.second; ++iter) {
//            if (iter->second == i) {
//               continue;
//            }
//
//	    potentialSimilarItems.insert(iter->second);
//         }
//      }

//      for (set<unsigned int>::const_iterator iter = potentialSimilarItems.begin();
//           iter != potentialSimilarItems.end();
//           ++iter) {

      for (unsigned int j = 0; j < itemsToUsers.size(); j++) {
         if (itemsToUsers[j].size() <= 0 || i == j) {
            continue;
         }

         similarItems.addItem(hashVectorSimilarity(i, j), j);
      }

      similarItems.print(i);
   }
}

vector<unsigned int> generateItemMinHash(unsigned int item)
{
   HashVector hashVector;

   vector<unsigned int> minHash;
   minHash.resize(NUM_HASHES);

   for (int i = 0; i < NUM_HASHES; i++) {
      minHash[i] = numeric_limits<unsigned int>::max();
   }

   for (unsigned int u = 0; u < itemsToUsers[item].size(); u++) {

      vector<unsigned int>userHash = hashVector.getHashVector(itemsToUsers[item][u]);
      
      for (int i = 0; i < NUM_HASHES; i++) {
         minHash[i] = min(minHash[i], userHash[i]);
      }
   }
   
   return minHash; 
}

void generateMinHashes()
{
   for (unsigned int i = 0; i < itemsToUsers.size(); i++) {

      vector<unsigned int> minHash = generateItemMinHash(i);
      itemsToMinHashes[i] = minHash;
      
      //for (int j = 0; j < NUM_HASHES; j++) {
      //   minHashesToItems[j].insert(pair<unsigned int, unsigned int>(minHash[j], i)); 
      //}
   }
   
}

int main()
{
   readInputFileIntoMap();
   generateMinHashes();
   generateAndOutputRecommendedItems();
   
   return 0; 
}
