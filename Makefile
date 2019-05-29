CXX = g++
CXXFLAGS = -std=c++14 -Wall -Werror `pkg-config --cflags libndn-cxx`
LIBS = `pkg-config --libs libndn-cxx`
DESTDIR ?= /usr/local

PROGRAMS = facemon file-server prefix-allocate prefix-request register-prefix-cmd serve-certs

all: $(PROGRAMS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

tap-tunnel: tap-tunnel*.cpp tap-tunnel*.hpp
	$(CXX) $(CXXFLAGS) -o $@ tap-tunnel*.cpp $(LIBS)

clean:
	rm -f $(PROGRAMS) tap-tunnel

install: all
	sh -c 'for P in $(PROGRAMS); do cp $$P $(DESTDIR)/bin/ndn6-$$P; done'

uninstall:
	sh -c 'for P in $(PROGRAMS); do rm -f $(DESTDIR)/bin/ndn6-$$P; done'
