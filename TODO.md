
General
-------

- Autosave.
- Save memory dumps, dissasembly...
- Allow to execute one frame per press.
- Cross out things in debugger that can't be used (transparent palette colors, GBA sprites that are hiden?, ...)
- Little screens in sprite viewers to indicate the position of a sprite?
- Update debugger windows when focusing the disassembler.
- Configure speedup key.

Game Boy
--------

- Switch LCD off effect (check on hardware).
- Emulate joypad bounce. This doesn't happen in GBA SP.
- When loading a GB only ROM in GBC mode, prepare palettes.
- Make that everything read FFh except HRAM when DMG DMA is enabled.
- Memory after OAM is an OAM mirror? Check in all GB models.
- What STAT mode is the GBC when the LCD is switched on. 0?
- Check what memory areas do OAM DMA copy from depending on hardware (DMG/GBC).
- What happens when TAC is changed while timer is working/not working in other models than the DMG/MGB.
- Initial register values (including hidden ones). Test with and without boot ROM.
- Initial memory values. Test with and without boot ROM.
- PPU state after powering on the screen. Is Any register reseted?
- When LCD is off, are LYC=0 or ScreenMode=0 IRQs triggered?
- Emulate weird behaviours of speed switch and STOP mode (screen color).
- Emulate RP register.
- P1 register starting value in DMG or GBC mode?
- Initial values in SGB/SGB2 of registers, timer and serial. I can't test because I don't have the hardware.

Game Boy Advance
----------------

- ARM undefined opcodes
- Sound glitches. (Buffer underflow or something?)
- Sprite limit (normal or affine) (with or without free HBL interval).
- The NDS7 and GBA allow to access CP14 (unlike as for CP0..CP13 & CP15, access to CP14 doesn't generate any exceptions)
- What happens with video modes 6 and 7?
- Emulate weird things with invalid window coordinates.
- Affine sprites/bgs(mode 7) + mosaic = bad
- Emulate mosaic effect correctly.
- Serial port.
- RTC. I/O registers for external hardware.
- The correct way of emulating is drawing a pixel every 4 clocks... But maybe it is too slow.
- Fix memory read (2 least significative bits)?
- Check ARM interpreter.
- Emulate pipeline to prevent programs from detecting they are running on an emulator?
- Fix CPU cycles.
- Configuration -> Disable GBA layers?
- THUMB LDMIA/STMIA: Strange Effects on Invalid Rlist's. Empty Rlist: R15 loaded/stored, and Rb=Rb+40h. Writeback with Rb included in Rlist: Store OLD base if Rb is FIRST entry in Rlist, otherwise store NEW base,no writeback.
- Caution: A very large OBJ (of 128 pixels vertically, ie. a 64 pixels OBJ in a Double Size area) located at Y>128 will be treated as at Y>-128, the OBJ is then displayed parts offscreen at the TOP of the display, it is then NOT displayed at the bottom.
- Video Capture Mode (DMA3 only): Intended to copy a bitmap from memory (or from external hardware/camera) to VRAM. When using this transfer mode, set the repeat bit, and write the number of data units (per scanline) to the word count register. Capture works similar like HBlank DMA, however, the transfer is started when VCOUNT=2, it is then repeated each scanline, and it gets stopped when VCOUNT=162.
- Rumble.
- Fix simulated SWI 11h and 12h (offset). (?)
- GBA_MemoryWrite8() will enter halt mode when writing to REG_POSTFLG. Fix it.
- Emulate broken sorting of priorities.
- What happens with prohibited code 3 in sprites?
- GBC DISPCNT bit can only be written from bios? Test.
- DMA Repeat bit (DMACNT 9) (Must be zero if Bit 11 set) - What happens if it is 1?

Random things
-------------

- Add a Makefile.
- [GB] Reverse engineer GB Camera. 
- [GB] Put wave RAM in the I/O viewer?
- [GBA] Final Fantasy Tactics Advance. Mateus Totema LY effect is wrong.
- [GBA] Reading a 32 bit value in ROM from EWRAM: 10 cycles (wait, wait, read instruction, compute address, wait, wait, wait, read low bits, wait, read high bits)
- [GBA] Reading a 32 bit value in EWRAM from EWRAM: 10 cycles (wait, wait, read instruction, compute address, wait, wait, read low bits, wait, wait, read high bits)
- [GBA] Loading a 16 bit value using mov/lsl/mov/orr: 12 cycles ( (wait, wait, read instruction) * 3 )



