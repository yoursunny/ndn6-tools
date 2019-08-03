CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Werror `pkg-config --cflags libndn-cxx`
LIBS ?= `pkg-config --libs libndn-cxx`
DESTDIR ?= /usr/local

PROGRAMS = facemon file-server prefix-allocate prefix-request register-prefix-cmd serve-certs

all: $(PROGRAMS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(PROGRAMS)

install: all
	sh -c 'for P in $(PROGRAMS); do cp $$P $(DESTDIR)/bin/ndn6-$$P; done'

uninstall:
	sh -c 'for P in $(PROGRAMS); do rm -f $(DESTDIR)/bin/ndn6-$$P; done'
