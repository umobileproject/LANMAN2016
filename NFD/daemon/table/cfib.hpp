
#ifndef NFD_DAEMON_TABLE_CFIB_HPP
#define NFD_DAEMON_TABLE_CFIB_HPP

#include "fib.hpp"

namespace nfd {

class Cfib : public Fib
{
public:
  explicit
  Cfib(NameTree& nameTree, size_t capacity);
  
  ~Cfib();
  
  std::pair<shared_ptr<fib::Entry>, bool>
  insert(const Name& prefix);

  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const Name& prefix);

  shared_ptr<fib::Entry>
  findExactMatch(const Name& prefix);

  //void
  //erase(const fib::Entry& entry);
  void
  erase(fib::Entry& entry);

  void 
  setCapacity(size_t capacity);

  size_t
  getCapacity();

private:
  template<class K, class T>
  struct LRUCacheEntry
  {
    K key;
    T data;
    LRUCacheEntry* prev;
    LRUCacheEntry* next;
  };

  template<class K, class T>
  class LRUCache
  {
  private:
    std::unordered_map< K, LRUCacheEntry<K,T>*, std::hash<ndn::Name>> m_mapping;
    std::vector< LRUCacheEntry<K,T>* >      m_freeEntries;
    LRUCacheEntry<K,T> *         m_head;
    LRUCacheEntry<K,T> *         m_tail;
    LRUCacheEntry<K,T> *         m_entries;
  public:
    LRUCache(size_t size)
    {
      m_entries = new LRUCacheEntry<K,T>[size];
      for (int i=0; i<size; i++)
        m_freeEntries.push_back(m_entries+i);
      m_head = new LRUCacheEntry<K,T>;
      m_tail = new LRUCacheEntry<K,T>;
      m_head->prev = NULL;
      m_head->next = m_tail;
      m_tail->next = NULL;
      m_tail->prev = m_head;
    }
    ~LRUCache()
    {
      delete m_head;
      delete m_tail;
      delete [] m_entries;
    }
    void setCap (size_t size)
    {
      //delete m_head;
      //delete m_tail;
      //delete m_entries;
      m_freeEntries.clear();
      m_entries = new LRUCacheEntry<K,T>[size];
      for (int i=0; i<size; i++)
        m_freeEntries.push_back(m_entries+i);
      m_head = new LRUCacheEntry<K,T>;
      m_tail = new LRUCacheEntry<K,T>;
      m_head->prev = NULL;
      m_head->next = m_tail;
      m_tail->next = NULL;
      m_tail->prev = m_head;
    }
    std::pair<T, bool> 
    put(K key, T data)
    {
       LRUCacheEntry<K,T>* node =m_mapping[key];
       if(node)
       {
         // refresh the link list
         detach(node);
         node->data = data;
         attach(node);
         return std::make_pair(data, false);
       }
       else
       {
         if ( m_freeEntries.empty() )
         {
            node = m_tail->prev;
            T entry = node->data;
            detach(node);
            m_mapping.erase(node->key);
            node->data = data;
            node->key = key;
            m_mapping[key] = node;
            attach(node);
            return std::make_pair(entry, true);
         }
         else
         {
           node = m_freeEntries.back();
           m_freeEntries.pop_back();
           node->key = key;
           node->data = data;
           m_mapping[key] = node;
           attach(node);
           return std::make_pair(data, false);
         }
       }
    }

    T remove (K const key)
    {
      LRUCacheEntry<K,T>* node = m_mapping[key];
      if(node)
      {
         detach(node);
         m_mapping.erase(node->key);
         m_freeEntries.push_back(node); 
         return node->data;
      }
      else 
        return NULL;
    }

    T get(K const key) 
    {
      LRUCacheEntry<K,T>* node = m_mapping[key];
      if(node)
      {
         detach(node);
         attach(node);
         return node->data;
      }
      else 
        return NULL;
    }

    private:
      void detach(LRUCacheEntry<K,T>* node)
      {
        node->prev->next = node->next;
        node->next->prev = node->prev;
      }
      void attach(LRUCacheEntry<K,T>* node)
      {
        node->next = m_head->next;
        node->prev = m_head;        m_head->next = node;
        node->next->prev = node;
      }
  }; //LRUCache

private:
  LRUCache<Name, shared_ptr<fib::Entry>> m_cache; // cache of FIB table m_entries
  size_t m_limit; // capacity of the FIB table 
};

} //namespace nfd

#endif
