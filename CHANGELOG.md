
                               Known bugs
                               ----------

- Sound glitches. (Buffer underflow or something?)
- Affine sprites/bgs(mode 7) + mosaic = bad
- GBA_MemoryWrite8() will enter halt mode when writing to REG_POSTFLG
- GBA CPU timings bad.

                            Version changelog
                            -----------------

Version 0.2.0
-------------

General

- Complete new GUI using SDL2 (cross-plataform).
- Drag and drop ROMs supported.
- Create screenshot and bios folders if they don't exist.
- Breakpoints in disassembler. [GB/GBA]
- CPU registers can be changed. [GB/GBA]
- In GBA mode, the debugger steps to the next instruction even in halt mode (executing as clocks as needed, one frame max). In GB mode not.
- Memory can be edited in memory viewers. ASCII characters shown. [GB/GBA]
- Disassembly shows "true" or "false" to know if the next instruction will be executed (if it is conditional). [GB/GBA]
- Disassembly shows little arrows in branches to know where they go (up/down). [GB/GBA]

GBA emulation

- Barrel shifter fixes.
- Undefined instructions in THUMB emulated (except that ones using r15 as operand).
- ldm/stm with writeback with base included in rlist hopefully fixed.
- Fixed DMA sound IRQ.
- SOUNDCNT_H can now be writen when sound is off.
- Special effects (blending, fade white or black) fixed.
- Transparent sprites are always transparent even when blending is disabled by the windows. Tested in hardware.
- GREENSWAP (04000002h) I/O register emulated.
- Disassembly improved.
- Disassembly shows I/O register names that are accesed by ldr/str instructions.
- Fixed a bug where some ALU instructions in arm mode would be executed with 0 clocks needed.

GB emulation

- DAA fixed (using a table extracted by testing all combinations on hardware).
- 'ld hl,sp+n' and 'add sp,n' flags fixed.
- CPU emulation rewriten. A lot more accurate, but SLOWER. It must be modified...
- '4x4 World Trophy (Europe) (En,Fr,De,Es,It)' works.
- Disassembly improved
- Disassembly shows I/O register names that are accesed by memory instructions.


Version 0.1.0
-------------

- First release.
- Win32 GUI.
- GB, GBC, SGB and GBA emulation. GBA compatibility is still a bit low.
- Sound problems in GBA mode.
- Noise channel not correctly emulated (GB and GBA).
