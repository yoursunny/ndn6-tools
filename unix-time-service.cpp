#include "common.hpp"

namespace ndn6 {
namespace unix_time_service {

using ndn::MetaInfo;

int
main()
{
  name::setConventionEncoding(name::Convention::TYPED);
  Face face;
  KeyChain keyChain;
  Name prefix = "/localhop/unix-time";
  face.setInterestFilter(InterestFilter(prefix, "<>{0}"),
    [&] (const auto&, const Interest& interest) {
      if (!interest.getCanBePrefix() || !interest.getMustBeFresh()) {
        return;
      }
      Data data(Name(prefix).appendTimestamp());
      data.setMetaInfo(MetaInfo().setFreshnessPeriod(1_ms));
      keyChain.sign(data);
      face.put(data);
    },
    abortOnRegisterFail);
  face.processEvents();
  return 0;
}

} // namespace unix_time_service
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::unix_time_service::main();
}
