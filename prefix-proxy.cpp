#include "common.hpp"
#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/security/certificate-fetcher-direct-fetch.hpp>
#include <ndn-cxx/security/validation-policy-command-interest.hpp>
#include <ndn-cxx/security/validation-policy-simple-hierarchy.hpp>

namespace ndn6 {
namespace prefix_server {

namespace mgmt = ndn::mgmt;
namespace security = ndn::security;

class ValidationPolicyPassInterest : public security::ValidationPolicy
{
public:
  explicit ValidationPolicyPassInterest(std::unique_ptr<security::ValidationPolicy> inner)
  {
    setInnerPolicy(std::move(inner));
  }

protected:
  void checkPolicy(const Data& data, const std::shared_ptr<security::ValidationState>& state,
                   const ValidationContinuation& continueValidation) override
  {
    getInnerPolicy().checkPolicy(data, state, continueValidation);
  }

  void checkPolicy(const Interest& interest,
                   const std::shared_ptr<security::ValidationState>& state,
                   const ValidationContinuation& continueValidation) override
  {
    Name klName = getKeyLocatorName(interest, *state);
    continueValidation(std::make_shared<security::CertificateRequest>(klName), state);
  }
};

static Face face;
static KeyChain keyChain;
static nfd::Controller controller(face, keyChain);
static security::Validator validator(
  std::make_unique<security::ValidationPolicyCommandInterest>(
    std::make_unique<ValidationPolicyPassInterest>(
      std::make_unique<security::ValidationPolicySimpleHierarchy>())),
  std::make_unique<security::CertificateFetcherDirectFetch>(face));
static mgmt::Dispatcher dispatcher(face, keyChain);
static nfd::RibRegisterCommand ribRegister;
static nfd::RibUnregisterCommand ribUnregister;

static void
authorize(const Name& prefix, const Interest& interest, const mgmt::ControlParameters* params0,
          const mgmt::AcceptContinuation& accept, const mgmt::RejectContinuation& reject)
{
  const auto& params = static_cast<const nfd::ControlParameters&>(*params0);
  if (!params.hasName()) {
    reject(mgmt::RejectReply::SILENT);
    return;
  }
  auto name = params.getName();

  Name signer;
  try {
    ndn::SignatureInfo si(
      interest.getName().at(ndn::signed_interest::POS_SIG_INFO).blockFromValue());
    if (si.hasKeyLocator() && si.getKeyLocator().getType() == tlv::Name) {
      signer = security::extractIdentityNameFromKeyLocator(si.getKeyLocator().getName());
    }
  } catch (const tlv::Error&) {
    reject(mgmt::RejectReply::SILENT);
    return;
  }
  if (!signer.isPrefixOf(name)) {
    reject(mgmt::RejectReply::STATUS403);
    return;
  }

  validator.validate(
    interest, [=](const Interest&) { accept(""); },
    [=](const Interest&, const security::ValidationError&) {
      reject(mgmt::RejectReply::STATUS403);
    });
}

static void
defineCommand(const nfd::ControlCommand* command, const ndn::PartialName& verb,
              const std::function<void(uint64_t, const nfd::ControlParameters&,
                                       const mgmt::CommandContinuation&)>& handler)
{
  dispatcher.addControlCommand<nfd::ControlParameters>(
    verb, authorize,
    [=](const auto& params) {
      try {
        command->validateRequest(static_cast<const nfd::ControlParameters&>(params));
        return true;
      } catch (const nfd::ControlCommand::ArgumentError&) {
        return false;
      }
    },
    [=](const Name& prefix, const Interest& interest, const mgmt::ControlParameters& params,
        const mgmt::CommandContinuation& done) {
      auto incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag>();
      if (incomingFaceIdTag == nullptr) {
        done(nfd::ControlResponse(400, "IncomingFaceId missing"));
        return;
      }
      handler(*incomingFaceIdTag, static_cast<const nfd::ControlParameters&>(params), done);
    });
}

template<typename Command>
static void
proxyCommand(uint64_t client, nfd::ControlParameters params, mgmt::CommandContinuation done)
{
  params.setFaceId(client);
  controller.start<Command>(
    params,
    [=](const nfd::ControlParameters& body) {
      nfd::ControlResponse res(200, "");
      res.setBody(body.wireEncode());
      done(res);
    },
    [=](const nfd::ControlResponse& res) { done(res); });
}

static void
handleRegister(uint64_t client, const nfd::ControlParameters& params,
               const mgmt::CommandContinuation& done)
{
  std::cout << "R\t" << client << '\t' << params.getName() << std::endl;
  proxyCommand<nfd::RibRegisterCommand>(client, params, done);
}

static void
handleUnregister(uint64_t client, const nfd::ControlParameters& params,
                 const mgmt::CommandContinuation& done)
{
  std::cout << "U\t" << client << '\t' << params.getName() << std::endl;
  proxyCommand<nfd::RibUnregisterCommand>(client, params, done);
}

int
main(int argc, char** argv)
{
  Name listenPrefix("/localhop/nfd");
  auto args = parseProgramOptions(
    argc, argv,
    "Usage: prefix-proxy\n"
    "\n"
    "Proxy prefix registration commands.\n"
    "\n",
    [&](auto addOption) {
      addOption("listen", po::value<Name>(&listenPrefix), "listen prefix");
      addOption("anchor", po::value<std::vector<std::string>>()->composing(), "trust anchor files");
    });

  if (args.count("anchor") > 0) {
    for (const std::string& filename : args["anchor"].as<std::vector<std::string>>()) {
      auto cert = io::load<Certificate>(filename);
      validator.loadAnchor(filename, std::move(*cert));
    }
  }

  enableLocalFields(controller, [&] {
    face.registerPrefix(Name(listenPrefix).append(ndn::PartialName("rib/register")), nullptr,
                        abortOnRegisterFail, SigningInfo(), nfd::ROUTE_FLAG_CAPTURE);
    face.registerPrefix(Name(listenPrefix).append(ndn::PartialName("rib/unregister")), nullptr,
                        abortOnRegisterFail, SigningInfo(), nfd::ROUTE_FLAG_CAPTURE);

    defineCommand(&ribRegister, "rib/register", handleRegister);
    defineCommand(&ribUnregister, "rib/unregister", handleUnregister);
    dispatcher.addTopPrefix(listenPrefix, false);
  });

  face.processEvents();
  return 0;
}

} // namespace prefix_server
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::prefix_server::main(argc, argv);
}
