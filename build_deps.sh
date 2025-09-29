# Builds this project's dependencies (our weston fork)
# and installs them to our local build folder in specific
# paths where they can then be referenced in our weston
# launcher.

BUILD_DIR=$(pwd)/build/weston-fork
INSTALL_DIR=${BUILD_DIR}/install

mkdir -p ${BUILD_DIR} ${INSTALL_DIR}

cd third_party/weston-fork
meson setup ${BUILD_DIR} --prefix=${INSTALL_DIR} --reconfigure
meson compile -C ${BUILD_DIR}
meson install -C ${BUILD_DIR}
