
#include "cfib.hpp"

namespace nfd {

NFD_LOG_INIT("Cfib");

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
  std::pair<shared_ptr<fib::Entry>, bool> p = Fib::insert(prefix); //returns true for a new nametable entry
  if(p.second)
  {
    std::pair<shared_ptr<fib::Entry>, bool> e = m_cache.put(p.first->getPrefix(), p.first); 
    if(e.second)
    {
      //NFD_LOG_INFO("Removed_SIT entry for "<<(e.first)->getPrefix().at(-1).toSequenceNumber()<<" due to space");
      //NFD_LOG_INFO("Removed_SIT entry for "<<(e.first)->getPrefix()<<" due to space");
      Fib::erase(*(e.first));  
    }
  }
  return p;
}

void
Cfib::erase(const fib::Entry& entry)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = getNameTree().get(entry);
  if (static_cast<bool>(nameTreeEntry)) { 
    //NFD_LOG_INFO("Removed_SIT entry for "<<nameTreeEntry->getPrefix().at(-1).toSequenceNumber());
    m_cache.remove(nameTreeEntry->getPrefix());
  }
  Fib::erase(entry);
}

} //namespace nfd
