
#include "cfib.hpp"

namespace nfd {

Cfib::Cfib(NameTree& nameTree, size_t capacity)
  : Fib(nameTree)
  , m_cache(capacity)
  , m_limit(capacity)
{
}

Cfib::~Cfib()
{
}

shared_ptr<fib::Entry>
Cfib::findLongestPrefixMatch(const Name& prefix)
{
  shared_ptr<fib::Entry> fibEntry = Fib::findLongestPrefixMatch(prefix);
  if(!Fib::isEmpty(fibEntry))
  {
    //update usage in the cache
    if(!m_cache.get(prefix))
    {
      std::cout << "This should not happen in Cfib findLongestPrefixMatch\n";
    }
  }
  return fibEntry;
}

shared_ptr<fib::Entry>
Cfib::findExactMatch(const Name& prefix)
{
  shared_ptr<fib::Entry> fibEntry = Fib::findExactMatch(prefix);
  if(static_cast<bool> (fibEntry) )
  {
    //update usage in the cache
    if(!m_cache.get(prefix))
    {
      std::cout << "This should not happen in Cfib findLongestPrefixMatch\n";
    }
  }

  return fibEntry;
}

std::pair<shared_ptr<fib::Entry>, bool>
Cfib::insert(const Name& prefix)
{
  std::pair<shared_ptr<fib::Entry>, bool> p = Fib::insert(prefix);
  if(p.second)
  {
    std::pair<shared_ptr<fib::Entry>, bool> e = m_cache.put(prefix, p.first); 
    if(e.second)
    {
      Fib::erase(*(e.first));  
    }
  }
  return p;
}

} //namespace nfd
