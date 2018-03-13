CXX = g++
CXXFLAGS = -std=c++11 -Wall -Werror `pkg-config --cflags libndn-cxx` -DBOOST_LOG_DYN_LINK
LIBS = `pkg-config --libs libndn-cxx`
DESTDIR ?= /usr/local

PROGRAMS = facemon prefix-allocate prefix-request register-prefix-cmd serve-certs tap-tunnel

all: $(PROGRAMS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

tap-tunnel: tap-tunnel*.cpp tap-tunnel*.hpp
	$(CXX) $(CXXFLAGS) -o $@ tap-tunnel*.cpp $(LIBS)

clean:
	rm -f $(PROGRAMS)

install: all
	sh -c 'for P in $(PROGRAMS); do cp $$P $(DESTDIR)/bin/ndn6-$$P; done'

uninstall:
	sh -c 'for P in $(PROGRAMS); do rm -f $(DESTDIR)/bin/ndn6-$$P; done'
