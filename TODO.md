
General
-------

- Autosave.
- Save memory dumps, dissasembly...
- Allow to execute one frame per press.
- Cross out things in debugger that can't be used (transparent palette colors, GBA sprites that are hiden?, ...)
- Little screens in sprite viewers to indicate the position of a sprite?
- Update debugger windows when focusing the disassembler.

Game Boy
--------

- Make that everything read FFh except HRAM when DMG DMA is enabled.
- HDMA copies when HALT is enabled? HALT is ignored if called when HDMA is active?
- Memory after OAM is an OAM mirror?
- Can disabled cart RAM be read?
- What STAT mode is the GBC when the LCD is switched on. 0?
- Check what memory areas do OAM DMA copy from depending on hardware (DMG/GBC).
- Check what happens if HDMA1-4 are modified during HDMA copy.
- HALT when IME=0. Halt not executed? Executed after IME is set to 1?
- HALT is disabled when any flag of IF is set to 1 or it needs the corresponding flag in IE? IME=1 only needed to jump to IRQ vectors?
- Start values are incorrect!
- When an interrupt occurs while in HALT, the CPU starts back up and pushes the Program Counter onto the stack before servicing the interrupt(s). Except it doesn't push the address after HALT as one might expect but rather the address of HALT itself?
- Reseting DIV affects serial? I doubt it, but better check it...
- What happens when TAC is changed while timer is working/not working.
- Switch speed needs a lot of clocks to complete. During that time some IRQs can be triggered! DIV counter keeps counting so all things that use that counter are affected

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



