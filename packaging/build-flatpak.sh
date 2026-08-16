#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MANIFEST="$ROOT/packaging/flatpak/io.github.mingistech.SpxViewer.yml"
BUNDLE="${BUNDLE:-spx-viewer-1.0.0-x86_64.flatpak}"

flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak-builder --user --install-deps-from=flathub --force-clean \
    --repo="$ROOT/.flatpak-repo" \
    "$ROOT/.flatpak-build" \
    "$MANIFEST"
flatpak build-bundle "$ROOT/.flatpak-repo" "$BUNDLE" io.github.mingistech.SpxViewer
echo "Wrote $BUNDLE"
