#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/certificate.hpp>
#include <ndn-cxx/util/io.hpp>

#include <cstdlib>
#include <iostream>

namespace ndn {
namespace serve_certs {

class ServeCerts : noncopyable
{
public:
  explicit
  ServeCerts(Face& face)
    : m_face(face)
  {
  }

  void
  add(shared_ptr<Data> data)
  {
    Name prefix = data->getName().getPrefix(-2); // omit IssuerId and Version components
    std::clog << "<R\t" << prefix << std::endl;
    m_face.setInterestFilter(prefix,
      [this, data] (const Name&, const Interest& interest) {
        std::clog << ">I\t" << interest << std::endl;
        if (interest.matchesData(*data)) {
          std::clog << "<D\t" << data->getName() << std::endl;
          m_face.put(*data);
        }
        else {
          auto nack = lp::Nack(interest).setReason(lp::NackReason::NO_ROUTE);
          std::clog << "<N\t" << interest << '~' << nack.getReason() << std::endl;
          m_face.put(nack);
        }
      },
      [] (const Name& prefix, const std::string& err) {
        std::cerr << "Unable to register " << prefix << ": " << err;
        std::exit(3);
      });
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
    app.add(io::load<security::v2::Certificate>(argv[i]));
  }

  face.processEvents();

  return 0;
}

} // namespace serve_certs
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::serve_certs::main(argc, argv);
}
