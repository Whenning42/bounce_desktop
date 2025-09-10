#!/usr/bin/env python3
# Takes a list of sleep durations and runs a process tree of height equal to the
# number of sleeps specified where the ith process lingers for the ith specified sleep
# duration.

import os
import subprocess
import sys
import time


def main():
    if len(sys.argv) < 2:
        print(
            "Usage: reaper_ptree.py <sleep_duration> [additional_args...]",
            file=sys.stderr,
        )
        sys.exit(1)

    # Pop the first argument as sleep_duration
    sleep_duration = int(sys.argv[1])
    remaining_args = sys.argv[2:]

    # If there are remaining arguments, run reaper_ptree.py with them
    if remaining_args:
        subprocess.Popen(
            [sys.executable, os.path.join(os.path.dirname(__file__), "reaper_ptree.py")]
            + remaining_args
        )

    # Sleep for sleep_duration if it's positive (as milliseconds)
    if sleep_duration > 0:
        time.sleep(sleep_duration / 1000.0)  # Convert milliseconds to seconds
    elif sleep_duration < 0:
        # If sleep_duration is negative, sleep forever
        while True:
            time.sleep(1)
    # If sleep_duration is 0, exit immediately


if __name__ == "__main__":
    main()
