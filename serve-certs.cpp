#include "common.hpp"

#include <ndn-cxx/security/certificate.hpp>

namespace ndn6 {
namespace serve_certs {

class ServeCerts : boost::noncopyable
{
public:
  explicit ServeCerts(Face& face)
    : m_face(face)
  {}

  void add(std::shared_ptr<Data> data)
  {
    Name prefix = data->getName().getPrefix(-2); // omit IssuerId and Version components
    std::cout << "<R\t" << prefix << std::endl;
    m_face.setInterestFilter(
      prefix,
      [this, data](const Name&, const Interest& interest) {
        std::cout << ">I\t" << interest << std::endl;
        if (interest.matchesData(*data)) {
          std::cout << "<D\t" << data->getName() << std::endl;
          m_face.put(*data);
        } else {
          auto nack = Nack(interest).setReason(lp::NackReason::NO_ROUTE);
          std::cout << "<N\t" << interest << '~' << nack.getReason() << std::endl;
          m_face.put(nack);
        }
      },
      abortOnRegisterFail);
  }

private:
  Face& m_face;
};

int
main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "serve-data cert-file cert-file..." << std::endl;
    return 2;
  }

  ndn::Face face;
  ServeCerts app(face);

  for (int i = 1; i < argc; ++i) {
    app.add(io::load<Certificate>(argv[i]));
  }

  face.processEvents();

  return 0;
}

} // namespace serve_certs
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::serve_certs::main(argc, argv);
}
