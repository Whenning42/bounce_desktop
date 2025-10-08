# Bounce Desktop

Bounce Desktop is a cpp library and python package for starting and interacting
with lightweight hardware-accelerated virtual desktops. It does this by running
VNC-backed Weston sessions and then connecting to them with a VNC client.

I've written Bounce Desktop to serve as a desktop platform for my RL with Games
framework: [BounceRL](https://github.com/Whenning42/bounce-rl).

# Dependencies

poetry, libvncserver (optional), gmock, gtest, libgvnc (from gtk-vnc)

# Getting started

If you want just want the python package, you'll first need to install the system
dependencies listed above. Then you can run this pip command:

```shell
pip install bounce_desktop
```

# Usage

I don't have docs or clear examples handy, but for starter pointers, see:
[src/desktop/client.h](src/desktop/client.h),
[bounce_desktop/bounce_desk_test.py](bounce_desktop/bounce_desk_test.py), and
[src/bindings/client_exe.h](src/bindings/client_ext.h).

# Limitations

Running multiple desktops from a single process isn't supported yet. I'd like to support
this use case, but there are some threading details to work through in the libgvnc
client and SDL viewer. Until then, if you do want to test the threaded set up,
pass in "true" to the the "allow_unsafe" arg for the client or viewer.

# Roadmap 

This project's being developed to support [BounceRL](https://github.com/Whenning42/bounce-rl),
so I'll be focusing development toward that project's needs. With that said, I
do think there's value in having a lightweight virtual desktop library available, and
I do think one could develop this project toward that goal if they're interested.

I also think this project should be close to feature complete at this point, I may at
some point integrate the prototyped, but unused, subprocess reaper into the desktop
class, so that callers can get guaranteed process clean-up, but we'll see if or when
I need that feature.

# Contributing

I don't plan on accepting pull requests in the near term, but I'll take a look at any
issues or feature requests you open.

