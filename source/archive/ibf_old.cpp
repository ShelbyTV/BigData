
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

//map<unsigned long, set<unsigned long> > usersToItems;
map<unsigned long, set<unsigned long> > itemsToUsers; 

// void printUsersToItems()
// {
//    for (map<unsigned long, set<unsigned long> >::const_iterator mapIter = usersToItems.begin();
//         mapIter != usersToItems.end(); 
//         ++mapIter) {
// 
//       cout << mapIter->first << endl;
// 
//       for (set<unsigned long>::const_iterator setIter = mapIter->second.begin();
//            setIter != mapIter->second.end();
//            ++setIter) {
// 
//          cout << "   " << *setIter << endl;
//       }
//    }
// }
// 
// void printItemsToUsers()
// {
//    for (map<unsigned long, set<unsigned long> >::const_iterator mapIter = itemsToUsers.begin();
//         mapIter != itemsToUsers.end(); 
//         ++mapIter) {
// 
//       cout << mapIter->first << endl;
// 
//       for (set<unsigned long>::const_iterator setIter = mapIter->second.begin();
//            setIter != mapIter->second.end();
//            ++setIter) {
// 
//          cout << "   " << *setIter << endl;
//       }
//    }
// }

double tanimoto(set<unsigned long>one, set<unsigned long>two)
{
   set<unsigned long>setIntersection;
   set<unsigned long>setUnion;

   set_intersection(one.begin(), one.end(), two.begin(), two.end(), inserter(setIntersection, setIntersection.end()));
   set_union(one.begin(), one.end(), two.begin(), two.end(), inserter(setUnion, setUnion.end()));

   return 1 - (setIntersection.size() / setUnion.size());
}

int main()
{
   ifstream input("input.csv");
 
   if (input.is_open()) {
      while (input.good()) {
         
         string line;   
         unsigned long user = 0, item = 0;
         
         getline(input, line);

         if (2 != sscanf(line.c_str(), "%lu,%lu", &user, &item)) {
            continue;
         }
            
//         map<unsigned long, set<unsigned long> >::iterator userIterator;
//         if ((userIterator = usersToItems.find(user)) != usersToItems.end()) {
//            userIterator->second.insert(item);
//         } else {
//            set <unsigned long> newSet;
//            newSet.insert(item);
//            usersToItems.insert(pair<unsigned long, set<unsigned long> >(user, newSet));
//         }

         map<unsigned long, set<unsigned long> >::iterator itemIterator;
         if ((itemIterator = itemsToUsers.find(item)) != itemsToUsers.end()) {
            itemIterator->second.insert(user);
         } else {
            set <unsigned long> newSet;
            newSet.insert(user);
            itemsToUsers.insert(pair<unsigned long, set<unsigned long> >(item, newSet));
         }
      }

      cout << "Loaded all .csv entries into 1 map." << endl;

      input.close();

      // user 9621 is onShelby Twitter account
      // loop through interesting items; could be all, or one user's 
//      map<unsigned long, set<unsigned long> >::iterator userIterator;
//      if ((userIterator = usersToItems.find(9621)) == usersToItems.end()) {
//         cout << "Could not find user 9621, the onShelby Twitter account" << endl;
//         return 0;
//      }
//
//      for (set<unsigned long>::const_iterator setIter = userIterator->second.begin();
//           setIter != userIterator->second.end();
//           ++setIter) {
//
//         multimap<double, unsigned long> similarItems;
//       
//         set<unsigned long> itemOneUserSet = itemsToUsers.find(*setIter)->second;
//         
//         for (map<unsigned long, set<unsigned long> >::const_iterator mapIter = itemsToUsers.begin();
//              mapIter != itemsToUsers.end(); 
//              ++mapIter) {
//            
//            if (mapIter->first == *setIter) {
//               continue;
//            }
// 
//            set<unsigned long> itemTwoUserSet = mapIter->second;
//            
//            similarItems.insert(pair<double, unsigned long>(tanimoto(itemOneUserSet, itemTwoUserSet), mapIter->first));
//         }
//
//         int count = 0;
//         
//         for (multimap<double, unsigned long>::const_iterator multimapIter = similarItems.begin();
//              multimapIter != similarItems.end() && count < 20;
//              ++multimapIter) {
//
//            cout << *setIter << " " << multimapIter->second << " " << multimapIter->first << endl;
//            count++;
//         } 
//
//         break;
//      }
       
      

//      cout << "UsersToItems:" << endl;
//      printUsersToItems();
//
//      cout << endl << "ItemsToUsers:" << endl;
//      printItemsToUsers();

   } else {
      cout << "Unable to open file"; 
   }
  
   return 0; 
}
