CXX ?= g++
CXXFLAGS ?= -Wall -Werror -Wno-error=deprecated-declarations -O2 -g
ALL_CXXFLAGS = $(CXXFLAGS) -std=c++17 `pkg-config --cflags libndn-cxx`
LDFLAGS ?=
LIBS ?= `pkg-config --libs libndn-cxx` -lboost_filesystem -lboost_program_options
PREFIX ?= /usr/local
DESTDIR ?=

PROGRAMS = \
	facemon \
	file-server \
	prefix-allocate \
	prefix-proxy \
	register-prefix-cmd \
	register-prefix-remote \
	serve-certs \
	unix-time-service

.PHONY: all
all: $(PROGRAMS)

%: %.cpp *.hpp
	$(CXX) $(ALL_CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

.PHONY: lint
lint:
	clang-format-15 -i *.hpp *.cpp

.PHONY: clean
clean:
	rm -f $(PROGRAMS)

.PHONY: install
install: all
	install -d -m0755 $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/lib/systemd/system
	sh -c 'for P in $(PROGRAMS); do install $$P $(DESTDIR)$(PREFIX)/bin/ndn6-$$P; done'
	install -m0644 -T serve-certs.service $(DESTDIR)$(PREFIX)/lib/systemd/system/ndn6-serve-certs.service

.PHONY: uninstall
uninstall:
	sh -c 'for P in $(PROGRAMS); do rm -f $(DESTDIR)$(PREFIX)/bin/ndn6-$$P; done'
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/ndn6-serve-certs.service
