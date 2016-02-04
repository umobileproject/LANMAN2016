#ifndef NDN_FLOODFLAG_HPP
#define NDN_FLOODFLAG_HPP

#include "encoding/block.hpp"
#include "encoding/block-helpers.hpp"

namespace ndn {


class FloodFlag 
{
public:

  /**
   * @brief Create a new Flood Flag with no components.
   */
  FloodFlag()
  //: m_dfBlock(tlv::FloodFlag) 
  {
    m_flag = 0;
  }

  FloodFlag(unsigned int flag)
  : m_flag(flag)
  {
  }

  uint32_t 
  get() const;

  void 
  set(uint32_t val);

  void
  clear();
  
private:
  uint32_t m_flag;
};

} // namespace ndn

#endif
