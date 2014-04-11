GiiBiiAdvance
=============

Another GB, GBC and GBA emulator writen in C. By Antonio Niño Díaz (AntonioND).

[http://antoniond_blog.drunkencoders.com/](http://antoniond_blog.drunkencoders.com/)

[http://antoniond.drunkencoders.com/](http://antoniond.drunkencoders.com/)

Source code:

[https://github.com/AntonioND/giibiiadvance](https://github.com/AntonioND/giibiiadvance)

This is an emulator I started a few years ago. I haven't released any version in years since 0.1.0, but I've done some changes that have improved compatibility anyway.

For 0.2.0 I've ported the Win32 GUI to SDL2. The objective is to make it portable, so the only windows outside the main one are the debugger windows. This way, if someone wanted to port this to a machine without window manager, the only thing he would need to do is to remove the debugger.

This has been compiled in Linux Mint 16 and Windows 7 succesfuly. Compiling with OpenCV in Linux is a bit tricky, and the emulator can't close nicely, but camera works. SDL windows handler is a bit buggy (?), so expect some glitches when using the debugger.

This is the only GB emulator that emulates completely the GB Camera if you have a webcam! :)

For 0.3.0
---------

- Improve the GUI:
 - Another subwindow to configure MBC7 and emulator controls like speedup.
 - GB Camera and GB Printer viewers. GB Camera: Registers, images (let the user see the thumbnails to choose).
 - GBA I/O hardware viewer (RTC, sensors...).
 - Export images from new debugger windows.
 - Dump dissasembly/memory to a file and restore it?
 - Wav recording.
 - Video recording?

- Obviously, improve emulation.
 - Fix sound.
 - Implement mosaic correctly (in GBA mode).
 - Correct GBA CPU timings.
 - Rewrite A LOT of GB core to speed up emulation.
 - Auto frameskip.
 - Fix broken x86 ASM instructions of GBA emulation in Linux. "setc (%%ebx)" seems to be the problem...
 - HuC3 and TAMA5 mappers for GB.
 
Dependencies
------------

This program needs SDL2 and OpenCV for GB Camera support. I'm using version 2.0.0 of OpenCV.

It also uses libpng and zlib, but at the moment they are statically linked so there are no conflicts between runtime and compiled versions.

