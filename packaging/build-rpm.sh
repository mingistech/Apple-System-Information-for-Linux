#!/bin/bash
set -euo pipefail

VERSION="${VERSION:-1.0.0}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOPDIR="${TOPDIR:-$HOME/rpmbuild}"

mkdir -p "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

tar -czf "$TOPDIR/SOURCES/spx-viewer-${VERSION}.tar.gz" \
    --transform "s,^,spx-viewer-${VERSION}/," \
    -C "$ROOT" \
    CMakeLists.txt LICENSE README.md src data screenshots

cp "$ROOT/packaging/spx-viewer.spec" "$TOPDIR/SPECS/"
rpmbuild --define "_topdir $TOPDIR" -ba "$TOPDIR/SPECS/spx-viewer.spec"

echo "RPMs written to $TOPDIR/RPMS and $TOPDIR/SRPMS"
