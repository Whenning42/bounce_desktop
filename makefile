# A makefile for building bounce_desktop along with its vendored version of weston.
# We use a makefile for building the vendored weston version, since our project requires
# we install weston to particular build locations during our build phase, and meson
# itself doesn't support running install steps at build time.

build: build_weston
	meson setup build/
	meson compile -C build/

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
