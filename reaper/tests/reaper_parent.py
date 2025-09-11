#!/usr/bin/env python3
# Test helper that launches the reaper executable with the given arguments,
# then sleeps for 100ms before exiting. Used to test parent exit scenarios.

import os
import subprocess
import sys
import time


def main():
    if len(sys.argv) < 3:
        print("Usage: reaper_parent.py <ipc_file> <reaper_args...>", file=sys.stderr)
        sys.exit(1)

    # Pass all arguments to the reaper executable
    ipc_file = sys.argv[1]
    reaper_args = sys.argv[2:]

    with open(ipc_file, "w") as f:
        f.write("0")
    env = os.environ
    env["REAPER_IPC_FILE"] = ipc_file

    # Launch the reaper with the provided arguments
    reaper_path = os.path.join(os.path.dirname(__file__), "..", "..", "build", "reaper")
    command = [reaper_path, "python3", "./reaper/tests/reaper_ptree.py", *reaper_args]
    p = subprocess.Popen(command, env=env)

    time.sleep(0.1)
    r = p.poll()
    if r is not None:
        print(f"reaper_parent.py exited with code: {r}")


if __name__ == "__main__":
    main()
