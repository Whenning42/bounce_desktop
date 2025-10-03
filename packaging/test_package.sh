# Builds and installs the python package into a new venv and runs
# the python tests.
#
# If --save_packge is set, replaces dist/* with the newly built version
# of this packge.

set -euo pipefail

# Check that script was run from correct folder, exit if not.
FILE="packaging/test_package.sh"
if [[ ! -e "$FILE" ]]; then
  echo "test_package.sh should be run from bounce_desktop's project root directory."
  exit 1
fi

# Parse options.
SAVE_PACKAGE=false
while [[ $# -gt 0 ]]; do
  case "$1" in
    --save_package) SAVE_PACKAGE=true; shift;;
    *) break;;
  esac
done

PROJECT_ROOT=$(pwd)
TEST_DIST=packaging/dist

# Set up the test venv.
rm -rf packaging/test_venv
(cd packaging; python -m venv test_venv)
source packaging/test_venv/bin/activate

# Build the package and save to packaging/dist
./packaging/build_package.sh ${TEST_DIST}

# Run tests under /tmp to prevent importing local files.
(cd /tmp && python ${PROJECT_ROOT}/bounce_desktop/bounce_desk_test.py)

# Copy the built package to dist if requested
if $SAVE_PACKAGE; then
  rm -rf dist/*
  mkdir -p dist/
  cp $TEST_DIST/* dist
fi

echo "Package build and test passed!"


