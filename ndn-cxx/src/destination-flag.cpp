
#include "destination-flag.hpp"

namespace ndn {
/*
template<encoding::Tag TAG>
size_t
DestinationFlag::wireEncode(EncodingImpl<TAG>& encoder) const
{
  size_t totalLength = prependNonNegativeIntegerBlock(encoder, tlv::DestinationFlag, m_flag);
  std::cout << "Encoding destination flag before send. Its value is: "<< m_flag <<"\n";
  return totalLength;
}

template size_t
DestinationFlag::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& encoder) const;

template size_t
DestinationFlag::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& encoder) const;

void
DestinationFlag::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::DestinationFlag)
    BOOST_THROW_EXCEPTION(tlv::Error("Unexpected TLV type when decoding DestinationFlag"));

  m_dfBlock = wire;
  m_flag = readNonNegativeInteger(wire);
  std::cout << "Decocoding destination flag after recv. Its value is: "<< m_flag <<"\n";
}

uint64_t
DestinationFlag::getFlag() const
{
  return m_flag;
}*/

void
DestinationFlag::set(uint32_t val)
{
  if(val >= 0)
    m_flag = val;
}

void 
DestinationFlag::clear()
{
  m_flag = 0;
}

uint32_t
DestinationFlag::get()
{
  return m_flag;
}

/*mutable Block & 
getDestinationFlagBlock()
{
  return m_dfBlock;
}*/

} // namespace ndn
