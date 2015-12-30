
#include "destination-flag.hpp"

namespace ndn {

template<encoding::Tag TAG>
size_t
DestinationFlag::wireEncode(EncodingImpl<TAG>& encoder) const
{
  size_t totalLength = prependNonNegativeIntegerBlock(encoder, tlv::DestinationFlag, m_flag);
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
}

uint64_t
DestinationFlag::getFlag() const
{
  return m_flag;
}

void
DestinationFlag::set()
{
  m_flag = 1;
}

void 
DestinationFlag::clear()
{
  m_flag = 0;
}

} // namespace ndn
