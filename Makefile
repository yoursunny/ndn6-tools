CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Werror `pkg-config --cflags libndn-cxx`
LDFLAGS ?=
LIBS ?= `pkg-config --libs libndn-cxx`
PREFIX ?= /usr/local
DESTDIR ?=

PROGRAMS = \
	facemon \
	file-server \
	prefix-allocate \
	prefix-proxy \
	prefix-request \
	register-prefix-cmd \
	register-prefix-remote \
	serve-certs \
	unix-time-service

all: $(PROGRAMS)

%: %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

lint:
	clang-format-8 -i -style=file *.hpp *.cpp

clean:
	rm -f $(PROGRAMS)

install: all
	install -d -m0755 $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/lib/systemd/system
	sh -c 'for P in $(PROGRAMS); do install $$P $(DESTDIR)$(PREFIX)/bin/ndn6-$$P; done'
	install -m0644 -T serve-certs.service $(DESTDIR)$(PREFIX)/lib/systemd/system/ndn6-serve-certs.service

uninstall:
	sh -c 'for P in $(PROGRAMS); do rm -f $(DESTDIR)$(PREFIX)/bin/ndn6-$$P; done'
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/ndn6-serve-certs.service
