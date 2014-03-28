GiiBiiAdvance
=============

Another GB, GBC and GBA emulator writen in C.

This is an emulator I started a few years ago. I haven't released any version since 0.1.0, but I've done some changes that have improved compatibility anyway.

I'm currently porting the Win32 GUI to SDL2. The objective is to make it portable, so the only windows outside the main one are the debugger windows. This way, if someone wanted to port this to a machine without window manager, the only thing he would need to do is to remove the debugger.

This has been compiled in Linux Mint 16 and Windows 7 succesfuly. Compiling with OpenCV in Linux is a bit tricky, and the emulator can't close nicely, but camera works.

When the current goals are done, the version 0.2.0 will be released. Until then even though the files say that it is version 0.2.0, it really isn't. It's 0.1.X.

This is the only GB emulator that emulates completely the GB Camera if you have a webcam! :)

Current goals for 0.2.0
-----------------------

- Finish porting the GUI:
 - Configuration window. Color select dialog?
 - Complete main window menu. Accelerators (F12, CTRL+R, ...)
 - Disable main menu elements when they can't be used.
 - Include font and app icon as binary data. Don't include icon data in linux.
 - Speedup button. Frameskip.

For 0.3.0
---------

- Improve the GUI:
 - GB Camera and GB Printer viewers. GB Camera: Registers, images (let the user see the thumbnails to choose).
 - GBA I/O hardware viewer (RTC, sensors...).
 - Export images from new debugger windows.
 - Dump dissasembly/memory to a file and restore it?

- Customize controls.
 - Support for game controllers.

- Obviously, improve emulation.
 - Implement mosaic correctly (in GBA mode).
 - Correct GBA CPU timings. Maybe not for version 0.2.0.
 - Rewrite a lot of GB core to speed up emulation.

Dependencies
------------

This program needs SDL2 and OpenCV for GB Camera support. I'm using version 2.0.0 of OpenCV.

It also uses libpng and zlib, but at the moment they are statically linked so there are no conflicts between runtime and compiled versions.

