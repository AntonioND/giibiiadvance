GiiBiiAdvance
=============

Another GB, GBC and GBA emulator.

This is an emulator I started a few years ago. I haven't released any version since 0.1.0, but I've done some changes that have improved compatibility anyway.

I'm currently porting the Win32 GUI to SDL2.

This has been compiled in Linux Mint 16 and Windows 7 succesfuly.

Current goals
-------------

- Finish GUI:
 - GBA I/O viewer
 - GB and GBA sprite viewer.
 - GB and GBA tile viewer.
 - GB and GBA palette viewer.
 - GB and GBA background viewer.
 - SGB viewer.
 - Configuration window.
 - Complete main window menu.
 - Export images from debugger windows.

- Customize controls.
 - Support for game controllers.

- Obviously, improve emulation.
 - Implement mosaic correctly.
 - Correct GBA CPU timings.
 - Rewrite a lot of GB core to speed up emulation.

Dependencies
------------

This program needs SDL2.

It also uses libpng and zlib, but at the moment they are statically linked so there are no conflicts between runtime and compiled versions.

