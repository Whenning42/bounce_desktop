# A makefile for building bounce_desktop along with its vendored version of weston.
#
# We use a makefile for all of our builds, since we use a particular set up for builds
# and installs that meson doesn't support well out of the box. Specifically, to unify
# our project layout between builds and installs, we only support running the project
# post-meson-install. We don't support post-builds at all and using a makefile for
# all builds makes this clearer / less error-prone.

BUILD_DIR := ${CURDIR}/build
build: build_weston
	meson setup build/ --prefix=${BUILD_DIR}
	meson install -C build/

WESTON_BUILD_DIR := ${CURDIR}/build/weston-fork
WESTON_TMP := ${CURDIR}/build/temp_weston
WESTON := ${CURDIR}/build/bounce_desktop/_vendored/weston
build_weston:
	cd subprojects/weston-fork; \
	meson setup ${WESTON_BUILD_DIR} --reconfigure --buildtype=debug \
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

test: build
	meson test -C build/
