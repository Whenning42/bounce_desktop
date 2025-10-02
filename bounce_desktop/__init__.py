"""BounceDesktop - Hardware-accelerated virtual desktops library."""

from pathlib import Path

__version__ = "0.0.2"

_package_dir = Path(__file__).parent

from ._core import Desktop

__all__ = ["Desktop"]
