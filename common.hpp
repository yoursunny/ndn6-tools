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
#include <ndn-cxx/util/span.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <cstdio>
#include <iostream>
#include <time.h>

namespace ndn6 {

namespace po = boost::program_options;

inline po::variables_map
parseProgramOptions(int argc, char** argv, const char* usage,
                    const std::function<void(po::options_description_easy_init)>& declare)
{
  po::options_description options("Options");
  auto addOption = options.add_options();
  addOption("help,h", "print help message");
  declare(addOption);

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  try {
    po::notify(vm);
  } catch (const po::error&) {
    std::cerr << usage << options;
    std::exit(2);
  }

  if (vm.count("help") > 0) {
    std::cout << usage << options;
    std::exit(0);
  }

  return vm;
}

using namespace ndn::literals::time_literals;

namespace io = ndn::io;
namespace lp = ndn::lp;
namespace name = ndn::name;
namespace nfd = ndn::nfd;
namespace time = ndn::time;
namespace tlv = ndn::tlv;

using ndn::Block;
using ndn::Data;
using ndn::Face;
using ndn::Interest;
using ndn::InterestFilter;
using ndn::KeyChain;
using ndn::Name;
using ndn::Scheduler;
using ndn::lp::Nack;
using ndn::security::Certificate;
using ndn::security::InterestSigner;
using ndn::security::SigningInfo;

inline void
enableLocalFields(nfd::Controller& controller, const std::function<void()>& then = nullptr)
{
  controller.start<nfd::FaceUpdateCommand>(
    nfd::ControlParameters().setFlagBit(nfd::FaceFlagBit::BIT_LOCAL_FIELDS_ENABLED, true),
    [&](const auto& cp) {
      std::cerr << "EnableLocalFields OK" << std::endl;
      if (then != nullptr) {
        then();
      }
    },
    [](const auto& cr) {
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
