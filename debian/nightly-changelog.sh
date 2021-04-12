#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..

PKGVER=$(git show -s --format='%ct %H' | gawk '{ printf "0.0.0-%s-%s", strftime("%Y%m%d%H%M%S", $1), substr($2,1,12) }')

(
  echo "ndn6-tools ($PKGVER-nightly~$DISTRO) $DISTRO;"
  echo "  * Automated build of version $PKGVER"
  echo " -- Junxiao Shi <deb@mail1.yoursunny.com>  $(date -R)"
) > debian/changelog
