#include "pick-one-strategy.hpp"
#include <boost/random/uniform_int_distribution.hpp>
#include <ndn-cxx/util/random.hpp>

namespace nfd {
namespace fw {
NFD_LOG_INIT("PickOneStrategy");


const Name PickOneStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/pickone");
NFD_REGISTER_STRATEGY(PickOneStrategy);

PickOneStrategy::PickOneStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

/** \brief determines whether a NextHop is eligible
 *  \param currentDownstream incoming FaceId of current Interest
 *  \param wantUnused if true, NextHop must not have unexpired OutRecord
 *  \param now time::steady_clock::now(), ignored if !wantUnused
 */
static inline bool
predicate_NextHop_eligible(const shared_ptr<pit::Entry>& pitEntry,
  const fib::NextHop& nexthop, FaceId currentDownstream,
  bool wantUnused = false,
  time::steady_clock::TimePoint now = time::steady_clock::TimePoint::min())
{
  shared_ptr<Face> upstream = nexthop.getFace();

  // upstream is current downstream
  if (upstream->getId() == currentDownstream)
    return false;

  // forwarding would violate scope
  if (pitEntry->violatesScope(*upstream))
    return false;

  if (wantUnused) {
    // NextHop must not have unexpired OutRecord
    pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(*upstream);
    if (outRecord != pitEntry->getOutRecords().end() &&
        outRecord->getExpiry() > now) {
      return false;
    }
  }

  return true;
}

static bool
canForwardToNextHop(shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return pitEntry->canForwardTo(*nexthop.getFace());
}

static bool
hasFaceForForwarding(const fib::NextHopList& nexthops, shared_ptr<pit::Entry>& pitEntry)
{
  return std::find_if(nexthops.begin(), nexthops.end(), bind(&canForwardToNextHop, pitEntry, _1))
         != nexthops.end();
}

void
PickOneStrategy::afterReceiveInterest(const Face& inFace,
                   const Interest& interest,
                   shared_ptr<fib::Entry> fibEntry,
                   shared_ptr<fib::Entry> sitEntry,
                   shared_ptr<pit::Entry> pitEntry)
{
  NFD_LOG_DEBUG("afterReceiveInterest interest=" << interest.getName());
  int sdc = pitEntry->getFloodFlag();
  uint32_t cost = 1;
  bool sent = false;
  if(fibEntry->hasNextHops())
  {
    cost = fibEntry->getNextHops()[0].getCost();
  }

  if(pitEntry->getDestinationFlag() && static_cast<bool>(sitEntry) /*&& sdc > 0*/)
  {//Destination Flag is set so follow  a randomly picked nexthop in SIT

    const fib::NextHopList& nexthops = sitEntry->getNextHops();

    // Ensure there is at least 1 Face is available for forwarding
    if (hasFaceForForwarding(nexthops, pitEntry)) {
      fib::NextHopList::const_iterator selected;
      do {
        boost::random::uniform_int_distribution<> dist(0, nexthops.size() - 1);
        const size_t randomIndex = dist(m_randomGenerator);

        uint64_t currentIndex = 0;

        for (selected = nexthops.begin(); selected != nexthops.end() && currentIndex != randomIndex;
           ++selected, ++currentIndex) {
        }
      } while (!canForwardToNextHop(pitEntry, *selected));
      sent = true;
      this->sendInterest(pitEntry, selected->getFace());
    }
  } 
  else if(!pitEntry->getDestinationFlag() && static_cast<bool>(sitEntry) && sdc > 0 && cost > 0) 
  {  //pick a random nexthop from SIT if possible and send a search packet 
    const fib::NextHopList& nexthops = sitEntry->getNextHops();

    // Ensure there is at least 1 Face is available for forwarding
    if (hasFaceForForwarding(nexthops, pitEntry)) {

      fib::NextHopList::const_iterator selected;
      do {
        boost::random::uniform_int_distribution<> dist(0, nexthops.size() - 1);
        const size_t randomIndex = dist(m_randomGenerator);

        uint64_t currentIndex = 0;

        for (selected = nexthops.begin(); selected != nexthops.end() && currentIndex != randomIndex;
           ++selected, ++currentIndex) {
        }
      } while (!canForwardToNextHop(pitEntry, *selected));
      sdc--;
      pitEntry->setFloodFlag(sdc);
      sent = true;
	   (*pitEntry).setDestinationFlag(); 
      this->sendInterest(pitEntry, selected->getFace());
	   (*pitEntry).clearDestinationFlag(); 
    }
  }
  if(!pitEntry->getDestinationFlag() && sdc > 0) 
  {
    const fib::NextHopList& nexthops = fibEntry->getNextHops();
    fib::NextHopList::const_iterator it = nexthops.end();
      
    it = std::find_if(nexthops.begin(), nexthops.end(),
        bind(&predicate_NextHop_eligible, pitEntry, _1, inFace.getId(),
             false, time::steady_clock::TimePoint::min()));

    if (it == nexthops.end()) {
        NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");
        NFD_LOG_INFO("Reject DF 0 (no eligible FIB next hop) interest=" << interest.getName());
    }
    else
    {
      shared_ptr<Face> outFace = it->getFace();
      sdc--;
      (*pitEntry).setFloodFlag(sdc);
      NFD_LOG_INFO("Forwarding DF 0 using FIB interest=" << interest.getName());
      sent = true;
      this->sendInterest(pitEntry, outFace);
      NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                          << " newPitEntry-to=" << outFace->getId());
    }
  } //if sdc > 0
  /*if (!sent) {
    this->rejectPendingInterest(pitEntry);
  }*/
}

} // namespace fw
} // namespace nfd
