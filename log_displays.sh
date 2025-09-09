#!/bin/bash

# Fetches the X and Wayland display names from our running weston session.
# It's necessary that we spawn our own program to gets these vars, since only apps
# running as children of the weston session have access these vars.
# Used by wayland_backend.h/.cpp.
echo $DISPLAY $WAYLAND_DISPLAY > ${LOG_DISPLAYS_PATH}

# Exit after our parent exits.
parent=$PPID
self=$$
tail --pid="$parent" -f /dev/null; kill "$self"
