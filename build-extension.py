#!/usr/bin/env python3
"""
Poetry build script for bounce_desktop. Calls "make build" and copies the built
"bounce_desktop" package to the expected output directory.
"""

import shutil
import subprocess


def echo(s):
    subprocess.call(["echo", s])


def run_command(*args, **kwargs):
    # Use echo instead of print for printing actions, since python and subprocess stdout
    # don't seem to be correctly merged by pip's verbose mode.
    echo(f"Calling: {' '.join(*args)} ===============")
    subprocess.call(*args, **kwargs)
    echo(f"End call: {' '.join(*args)} ===============")


def build():
    run_command(["make", "build"])
    # Replace the sdist bounce_desktop dir with the built bounce_desktop dir.
    shutil.rmtree("bounce_desktop")
    shutil.copytree("build/bounce_desktop", "bounce_desktop")


if __name__ == "__main__":
    build()
