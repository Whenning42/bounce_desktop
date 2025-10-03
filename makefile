# A makefile for building bounce_desktop library, python package, and vendored weston.
#
# Commands:
#   make build: Builds the whole project and installs it under build/
#   make build_weston: Builds vendored weston and installs it under build/
#   make test: Runs the project's unit tests
#   make package: Builds the project's python package and stores it under dist/
#   make package_test: Runs the project's unit tests and builds and verifies the
#                      python package.
#   make upload: Run the project's unit and packaging tests and then uploads
#                the package to pypi.
#
#
# Note: We use a makefile for all of our builds, since:
# 1. We only support running this project in a locally installed configuration, so
#    we don't want users running "meson compile ..."
# 2. It gives us a convenient place to unify and document the build, package, test,
#    and upload commands our project uses.

BUILD_DIR := ${CURDIR}/build
build: build_weston
	meson setup build/ --prefix=${BUILD_DIR}
	meson install -C build/

WESTON_BUILD_DIR := ${CURDIR}/build/weston-fork
WESTON_TMP := ${CURDIR}/build/temp_weston
WESTON := ${CURDIR}/build/bounce_desktop/_vendored/weston
build_weston:
	cd subprojects/weston-fork; \
	meson setup ${WESTON_BUILD_DIR} --reconfigure --buildtype=release \
		--prefix=${WESTON_TMP} \
		-Dwerror=false \
		-Dbackend-vnc=true \
		-Drenderer-gl=true \
		-Dbackend-headless=true \
		-Dbackend-default=headless \
		-Drenderer-vulkan=false \
		-Dbackend-drm=false \
		-Dbackend-wayland=false \
		-Dbackend-x11=false \
		-Dbackend-rdp=false \
		-Dremoting=false \
		-Dpipewire=false
	meson compile -C ${WESTON_BUILD_DIR}
	meson install -C ${WESTON_BUILD_DIR}
	mkdir -p ${WESTON}
# We build in a tempory directory and then copy over to our real target directory
# with a "cp -R -L" so that we can convet symlinked .so's to copies of the .so's,
# since python doesn't support symlinks in sdists.
	cp -R -L ${WESTON_TMP}/. ${WESTON}
	rm -rf ${WESTON_TMP}

package: build
	./packaging/build_package.sh

test: build
	meson test -C build/

package_test: test
	./packaging/test_package.sh

upload: test
	./packaging/test_package.sh --save_package
	twine upload dist/*
