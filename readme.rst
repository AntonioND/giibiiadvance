GiiBiiAdvance
=============

Another GB, GBC and GBA cross-platform emulator writen in C. By Antonio Niño
Díaz (AntonioND). It's licensed under the GPL v2 license. The source code is
available `in GitHub <https://github.com/AntonioND/giibiiadvance>`_.

This is an emulator I started several years ago. I didn't release any version in
a few years after 0.1.0. At some point I did some changes that improved
compatibility a lot, and ported the original Win32 GUI to SDL2.  The objective
was to make it as portable as possible. Everything you need to play is in the
main window. The debugger needs additiona windows, but that's not needed to
play. If your machine doesn't have a windows manager, all the changes that are
needed are to disable the debugger.

This emulator isn't meant to be used for playing. It is perfectly
capable of emulating most GB/GBC games pretty accurately, and most GBA games
with reasonable accuracy. However, there are several emulators that may be a
probably a better idea than GiiBiiAdvance due to their better user support.

- `mGBA <https://mgba.io/>`_
- `no$gba <http://problemkaputt.de/gba.htm>`_
- `bgb <http://bgb.bircd.org>`_
- `Gambatte <https://github.com/sinamas/gambatte>`_

However, this is the first emulator that emulated completely the GB Camera (if
you have a webcam)! :) More information aobut the GB Camera hardware
`here <https://github.com/AntonioND/gbcam-rev-engineer>`_.

My website: www.skylyrac.net

Build instructions (Linux)
--------------------------

This program depends on CMake, SDL2, libpng and zlib.

.. code:: bash

    sudo apt install cmake libsdl2-dev libpng-dev

In order to built the program with the default set of options, type:

.. code:: bash

    mkdir build
    cd build
    cmake ..
    make

To build it faster, do a parallel build with:

.. code:: bash

    make -j`nproc`

If you want to build the emulator with GB Camera support, you need to install
OpenCV 4 as well:

.. code:: bash

    sudo apt install libopencv-dev

CMake will detect it during the build process and compile the emulator with Game
Boy Camera support.

Planned features:
-----------------

- Improve the GUI:

  - Another subwindow to configure MBC7 and emulator controls like speedup.
  - GB Camera and GB Printer viewers. GB Camera: Registers, images (let the user
    see the thumbnails to choose).
  - GBA I/O hardware viewer (RTC, sensors...).
  - Export images from new debugger windows.
  - Dump dissasembly/memory to a file and restore it?
  - Wav recording.
  - Video recording?

- Obviously, improve emulation.

  - Fix sound.
  - Implement mosaic correctly (in GBA mode).
  - Correct GBA CPU timings.
  - Rewrite A LOT of GB core to speed up emulation. (In progress)
  - Auto frameskip.
  - Fix broken x86 ASM instructions of GBA emulation in Linux. ``setc (%%ebx)``
    seems to be the problem...
  - HuC3, MMM01 and TAMA5 mappers for GB.
