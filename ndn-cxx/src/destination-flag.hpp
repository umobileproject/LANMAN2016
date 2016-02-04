#ifndef NDN_DESTINATIONFLAG_HPP
#define NDN_DESTINATIONFLAG_HPP

#include "encoding/block.hpp"
#include "encoding/block-helpers.hpp"

namespace ndn {

class DestinationFlag 
{
public:

  /**
   * @brief Create a new Destination Flag with no components.
   */
  DestinationFlag()
  //: m_dfBlock(tlv::DestinationFlag) 
  {
    m_flag = 0;
  }

  DestinationFlag(unsigned int flag)
  : m_flag(flag)
  {
  }
  /**
   * @brief Create RoutingName object for src_addr and dst_addr
  RoutingName(std::string name);
	*/

  //template<encoding::Tag TAG>
  //size_t
  //wireEncode(EncodingImpl<TAG>& encoder) const;

  //void
  //wireDecode(const Block& wire);

  //uint64_t
  //getFlag() const;
  
  uint32_t 
  get() const;

  void 
  set(uint32_t val);

  void
  clear();
  
  //mutable Block & 
  //getDestinationFlagBlock()

private:
  uint32_t m_flag;
};

} // namespace ndn

#endif
