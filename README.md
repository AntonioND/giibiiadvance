GiiBiiAdvance
=============

Another GB, GBC and GBA emulator writen in C.

This is an emulator I started a few years ago. I haven't released any version since 0.1.0, but I've done some changes that have improved compatibility anyway.

I'm currently porting the Win32 GUI to SDL2.

This has been compiled in Linux Mint 16 and Windows 7 succesfuly. Compiling with OpenCV hasn't been tested in Linux, but it should work.

When the current goals are done, the version 0.2.0 will be released. Until then even though the files say that it is version 0.2.0, it really isn't. It's 0.1.X.

Current goals
-------------

- Finish porting the GUI:
 - GBA I/O viewer
 - GBA sprite viewer.
 - GB and GBA tile viewer.
 - GB and GBA background viewer.
 - SGB viewer.
 - Configuration window.
 - Complete main window menu. Accelerators (F12, CTRL+R, ...)
 - Export images from debugger windows.

- Customize controls.
 - Support for game controllers.

- Obviously, improve emulation.
 - Correct GB Camera brightness (maybe).
 - Implement mosaic correctly (in GBA mode).
 - Correct GBA CPU timings. Maybe not for version 0.2.0.
 - Rewrite a lot of GB core to speed up emulation. NOT for version 0.2.0, maybe 0.3.0.

Dependencies
------------

This program needs SDL2 and OpenCV for GB Camera support. I'm using version 2.0.0 of OpenCV.

It also uses libpng and zlib, but at the moment they are statically linked so there are no conflicts between runtime and compiled versions.

