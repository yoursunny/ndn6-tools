# syntax=docker/dockerfile:1

ARG NDN_CXX_VERSION=latest
FROM ghcr.io/named-data/ndn-cxx-build:${NDN_CXX_VERSION} AS build

RUN --mount=rw,target=/src <<EOF
    set -eux
    cd /src
    make -j
    make install
EOF


FROM ghcr.io/named-data/ndn-cxx-runtime:${NDN_CXX_VERSION} AS ndn6-tools

COPY --link --from=build /usr/local/bin/ndn6-* /usr/bin/

ENV HOME=/config
VOLUME /config
VOLUME /run/nfd

ENTRYPOINT ["/bin/bash"]