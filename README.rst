GiiBiiAdvance
=============

Another GB, GBC and GBA cross-platform emulator writen in C. By Antonio Niño
Díaz (AntonioND). It's licensed under the GPL v2 license. The source code is
available `in GitHub <https://github.com/AntonioND/giibiiadvance>`_.

This is an emulator I started several years ago. I didn't release any version
for a few years after 0.1.0. At some point I did some changes that improved
compatibility a lot, and ported the original Win32 GUI to SDL2. The objective
was to make it as portable as possible. Everything you need to play is in the
main window. The debugger needs additiona windows, but that's not needed to
play. If your machine doesn't have a windows manager, all the changes that are
needed are to disable the debugger.

A few years later I decided to clean up the code and release 0.3.0. It has a lot
of improvements of the months after 0.2.0. It also has some bugfixes added
during a few years. Finally, the code has been cleaned up and a new build system
has been added so that it can finally be built in Windows easily.

This emulator isn't meant to be used for playing. It is perfectly
capable of emulating most GB/GBC games pretty accurately, and most GBA games
with reasonable accuracy. However, there are several emulators that probably are
a better idea than GiiBiiAdvance due to their better support.

- `mGBA <https://mgba.io/>`_
- `no$gba <http://problemkaputt.de/gba.htm>`_
- `bgb <http://bgb.bircd.org>`_
- `Gambatte <https://github.com/sinamas/gambatte>`_

However, this is the first emulator that emulated completely the GB Camera (if
you have a webcam)! :) More information aobut the GB Camera hardware
`here <https://github.com/AntonioND/gbcam-rev-engineer>`_.

It's also what I used to implement the PC simulation side of
`ugba <https://github.com/AntonioND/ugba>`_, my GBA library. For example, all
the video simulation and BIOS services.

My website: www.skylyrac.net

Build instructions for Linux
----------------------------

This program depends on CMake, SDL2 (at least version 2.0.7), libpng (at least
version 1.6) and zlib.

.. code:: bash

    sudo apt install cmake libsdl2-dev libpng-dev

If you want to build the emulator with GB Camera support, you need to install
OpenCV 4 as well. If OpenCV is detected during the build process, it will be
used. If not, the emulator won't have webcam support.

.. code:: bash

    sudo apt install libopencv-dev

If you want to support Lua scripts, you need to install Lua 5.2 or higher as
well. If it is detected during the build process, it will be used. If not, the
emulator won't have Lua scripts support. For example:

.. code:: bash

    sudo apt install liblua5.3-dev

In order to build GiiBiiAdvance with the default set of options, type:

.. code:: bash

    mkdir build
    cd build
    cmake ..
    make

To build it faster, do a parallel build with:

.. code:: bash

    make -j`nproc`

For a release build:

.. code:: bash

    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j`nproc`

Build instructions for Windows (Microsoft Visual Studio)
--------------------------------------------------------

In order to build with MinGW or Cygwin, you should use the Linux instructions.
The following instructions have been tested with Microsoft Visual C++ 2019.

You need to install `vcpkg`_. In short, open a PowerShell window and do:

.. code:: bash

    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    .\bootstrap-vcpkg.bat
    .\vcpkg integrate install --triplet x64-windows

This program depends on CMake, SDL2, libpng and zlib:

.. code:: bash

    .\vcpkg install SDL2 libpng --triplet x64-windows

If you want to build the emulator with GB Camera support, you need to install
OpenCV 4 as well. If OpenCV is detected during the build process, it will be
used. If not, the emulator won't have webcam support.

.. code:: bash

    .\vcpkg install opencv4 --triplet x64-windows

If you want to support Lua scripts, you need to install Lua 5.2 or higher as
well. If it is detected during the build process, it will be used. If not, the
emulator won't have Lua scripts support.

.. code:: bash

    .\vcpkg install liblua --triplet x64-windows

In order to build GiiBiiAdvance with the default set of options, type the
following commands in a Visual Studio command line shell (replacing the path to
the ``vcpkg`` folder by the one in your system):

.. code:: bash

    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\...\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
    msbuild GiiBiiAdvance.sln

In order to get a Release build, do:

.. code:: bash

    msbuild GiiBiiAdvance.sln /property:Configuration=Release

Instead of running msbuild from the command line, you can also open the solution
file with Visual Studio and build it from the IDE.

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

  - Implement mosaic correctly (in GBA mode).
  - Correct GBA CPU timings.
  - Rewrite A LOT of GB core to speed up emulation. (In progress)
  - Auto frameskip.
  - Fix broken x86 ASM instructions of GBA emulation in Linux. ``setc (%%ebx)``
    seems to be the problem...
  - HuC3, MMM01 and TAMA5 mappers for GB.

.. _vcpkg: https://github.com/microsoft/vcpkg
