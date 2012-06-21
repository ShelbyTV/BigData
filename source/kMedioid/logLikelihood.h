#ifndef __LOG_LIKELIHOOD_H__
#define __LOG_LIKELIHOOD_H__

#include <vector>

class LogLikelihood
{

public:

   static double ratio(const std::vector<unsigned int> &one,
                       const std::vector<unsigned int> &two,
                       const unsigned int total);

private:

   static double xLogX(const unsigned int x);

   static double entropy(const unsigned int a, const unsigned int b);
   static double entropy(const unsigned int a, const unsigned int b, 
                         const unsigned int c, const unsigned int d);

};

#endif // __LOG_LIKELIHOOD_H__
