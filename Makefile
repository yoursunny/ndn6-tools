CXX ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Werror `pkg-config --cflags libndn-cxx`
LDFLAGS ?=
LIBS ?= `pkg-config --libs libndn-cxx`
DESTDIR ?= /usr/local

PROGRAMS = \
	facemon \
	file-server \
	prefix-allocate \
	prefix-request \
	register-prefix-cmd \
	register-prefix-remote \
	serve-certs \
	unix-time-service

all: $(PROGRAMS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f $(PROGRAMS)

install: all
	sh -c 'for P in $(PROGRAMS); do cp $$P $(DESTDIR)/bin/ndn6-$$P; done'

uninstall:
	sh -c 'for P in $(PROGRAMS); do rm -f $(DESTDIR)/bin/ndn6-$$P; done'

install-serve-certs-service: serve-certs
	install -d -m0755 /usr/local/lib/systemd/system
	install -m0644 -T serve-certs.service /usr/local/lib/systemd/system/ndn6-serve-certs.service
	install -d -m0755 -ondn -gndn /var/lib/ndn/serve-certs
	systemctl daemon-reload

uninstall-serve-certs-service:
	systemctl disable ndn6-serve-certs || true
	systemctl stop ndn6-serve-certs || true
	rm /usr/local/lib/systemd/system/ndn6-serve-certs.service
	systemctl daemon-reload
