#ifndef NFD_DAEMON_FW_PICKONE_STRATEGY_HPP
#define NFD_DAEMON_FW_PICKONE_STRATEGY_HPP

#include <boost/random/mersenne_twister.hpp>
#include "strategy.hpp"

namespace nfd {
namespace fw {

class PickOneStrategy : public Strategy {
public:
  PickOneStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<fib::Entry> sitEntry,
                       shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE;
public:
  static const Name STRATEGY_NAME;
protected:
  boost::random::mt19937 m_randomGenerator;

};

} //namespace fw
} //namespace nfd
#endif //NDNSIM_PICK_ONE_FROM_SIT_TABLE_STRATEGY
