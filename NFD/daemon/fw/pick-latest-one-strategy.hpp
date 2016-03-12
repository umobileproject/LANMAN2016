#ifndef NFD_DAEMON_FW_PICKLATESTONE_STRATEGY_HPP
#define NFD_DAEMON_FW_PICKLATESTONE_STRATEGY_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

class PickLatestOneStrategy : public Strategy {
public:
  PickLatestOneStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<fib::Entry> sitEntry,
                       shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE;
public:
  static const Name STRATEGY_NAME;

};

} //namespace fw
} //namespace nfd
#endif //NDNSIM_PICK_ONE_FROM_SIT_TABLE_STRATEGY
