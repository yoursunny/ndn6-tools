#ifndef NDN6_TOOLS_COMMON_HPP
#define NDN6_TOOLS_COMMON_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/security/certificate.hpp>
#include <ndn-cxx/security/interest-signer.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <cstdio>
#include <iostream>
#include <time.h>

namespace ndn6 {

using namespace ndn::literals::time_literals;

namespace io = ndn::io;
namespace lp = ndn::lp;
namespace name = ndn::name;
namespace nfd = ndn::nfd;
namespace tlv = ndn::tlv;
namespace time = ndn::time;

namespace po = boost::program_options;

using ndn::Block;
using ndn::Name;
using ndn::Interest;
using ndn::Data;
using ndn::lp::Nack;

using ndn::Scheduler;
using ndn::Face;
using ndn::InterestFilter;

using ndn::KeyChain;
using ndn::security::Certificate;
using ndn::security::InterestSigner;
using ndn::security::SigningInfo;

inline void
enableLocalFields(ndn::nfd::Controller& controller, const std::function<void()>& then = nullptr)
{
  controller.start<ndn::nfd::FaceUpdateCommand>(
    ndn::nfd::ControlParameters()
      .setFlagBit(ndn::nfd::FaceFlagBit::BIT_LOCAL_FIELDS_ENABLED, true),
    [&] (const auto& cp) {
      std::cerr << "EnableLocalFields OK" << std::endl;
      if (then != nullptr) {
        then();
      }
    },
    [] (const auto& cr) {
      std::cerr << "EnableLocalFields error " << cr << std::endl;
      std::exit(1);
    });
}

inline void
abortOnRegisterFail(const Name& name, const std::string& message)
{
  std::cerr << "RegisterPrefix error " << name << " " << message << std::endl;
  std::exit(1);
}

} // namespace ndn6

#endif // NDN6_TOOLS_COMMON_HPP
