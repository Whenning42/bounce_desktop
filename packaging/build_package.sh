# Builds the bounce_desktop sdist.
#
# Usage:
# $ build_package.sh [optional custom dist dir]

set -euo pipefail

usage() { echo "usage: $(basename "$0") [optional_custom_dist_dir]"; }
# allow at most one positional arg
if (( $# > 1 )); then usage; exit 2; fi

DIST=${1:-"dist"}
mkdir -p ${DIST}
rm -rf ${DIST}/*
poetry build -f sdist -o ${DIST} && pip install ${DIST}/bounce_desktop-*.tar.gz
