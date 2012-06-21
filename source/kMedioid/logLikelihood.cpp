#include <algorithm>
#include <set>
#include <cmath>

#include "logLikelihood.h"

using namespace std;

double LogLikelihood::xLogX(unsigned int x)
{
   return x == 0 ? 0.0 : (double)x * log((double)x);
}

double LogLikelihood::entropy(unsigned int a, 
                              unsigned int b)
{
   return xLogX(a + b) - xLogX(a) - xLogX(b);
}

double LogLikelihood::entropy(unsigned int a, 
                              unsigned int b, 
                              unsigned int c, 
                              unsigned int d)
{
   return xLogX(a + b + c + d) - xLogX(a) - xLogX(b) - xLogX(c) - xLogX(d);
}

double LogLikelihood::ratio(const vector<unsigned int> &one, 
                            const vector<unsigned int> &two,
                            const unsigned int total)
{
   unsigned int numUsersWhoSharedOne = one.size();
   unsigned int numUsersWhoSharedTwo = two.size();

   unsigned int intersectionCount = 0;
   vector<unsigned int>::const_iterator iterOne = one.begin();
   vector<unsigned int>::const_iterator iterTwo = two.begin();
   vector<unsigned int>::const_iterator iterOneEnd = one.end();
   vector<unsigned int>::const_iterator iterTwoEnd = two.end();

   // faster than STL set_intersection, since we're just keeping a count
   while (iterOne != iterOneEnd && iterTwo != iterTwoEnd) {
      if (*iterOne < *iterTwo) {
         ++iterOne;
      } else if (*iterTwo < *iterOne) {
         ++iterTwo;
      } else {
         ++iterOne;
         ++iterTwo;
         ++intersectionCount;
      }
   }

   unsigned int k11 = intersectionCount; // both together
   unsigned int k12 = numUsersWhoSharedTwo - k11; // two, not one
   unsigned int k21 = numUsersWhoSharedOne - k11; // one, not two
   unsigned int k22 = total - (k12 + k21 + k11); // otherwise

   double rowEntropy = entropy(k11, k12) + entropy(k21, k22);
   double columnEntropy = entropy(k11, k21) + entropy(k12, k22);
   double matrixEntropy = entropy(k11, k12, k21, k22);

   return 2.0 * max(0.0, (matrixEntropy - rowEntropy - columnEntropy));
}
