#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <cstdlib>
#include <iostream>

int
main()
{
  ndn::name::setConventionEncoding(ndn::name::Convention::TYPED);
  ndn::Face face;
  ndn::KeyChain keyChain;
  ndn::Name prefix = "/localhop/unix-time";
  face.setInterestFilter(ndn::InterestFilter(prefix, "<>{0}"),
    [&] (const auto&, const ndn::Interest& interest) {
      if (!interest.getCanBePrefix() || !interest.getMustBeFresh()) {
        return;
      }
      ndn::Data data(ndn::Name(prefix).appendTimestamp());
      data.setMetaInfo(ndn::MetaInfo().setFreshnessPeriod(ndn::time::milliseconds(1)));
      keyChain.sign(data);
      face.put(data);
    },
    [] (const ndn::Name&, const std::string& err) {
      std::cerr << err << std::endl;
      std::exit(1);
    });
  face.processEvents();
  return 0;
}
