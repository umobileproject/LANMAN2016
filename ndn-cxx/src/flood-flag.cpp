
#include "flood-flag.hpp"

namespace ndn {

void
FloodFlag::set(uint32_t val)
{
  if(val >= 0)
    m_flag = val;
}

void 
FloodFlag::clear()
{
  m_flag = 0;
}

uint32_t
FloodFlag::get()
{
  return m_flag;
}

} // namespace ndn
