
General
-------

- ARM undefined opcodes
- Autosave.
- All missing things from version 0.1.0 Win32 GUI.
- Update README file.
- Update memory, I/O viewers... with F7.
- Save tilesets, maps, memory dumps...
- Allow to execute one frame.
- Pause/unpause.
- Cross out unused palette colors in the palette viewer.
- Better sprite viewer.
- Detect ROM (GB/GBA) checking the headers?.
- Custom controls. Game controllers.

Game Boy
--------

- Measure delay between VBL and VBL IRQ.
- Make that everything reads FFh except HRAM when DMG DMA is enabled.
- Check GDMA/HDMA timings in real hardware.
- Fix reset in GBC mode. (?)
- Memory after OAM is an OAM mirror?
- What happens if [DMA] = FFh or any other invalid value?
- Put wave RAM in the I/O viewer?
- Rumble.
- GB Camera.
- ATTRACTION MODE (SGB). Maybe it is the thing that the system borders can animate in a real SGB.

Game Boy Advance
----------------

- What happens with video modes 6 and 7?
- Emulate weird things with invalid window coordinates.
- Fix sound.
- Serial port.
- RTC. I/O registers for external hardware.
- The correct way of emulating is drawing a pixel every 4 clocks... But maybe it is too slow.
- Fix memory read (2 least significative bits)
- Check ARM interpreter.
- Emulate pipeline to prevent programs from detecting they are running on an emulator.
- Fix CPU cycles.
- Configuration -> Disable GBA layers?
- THUMB LDMIA/STMIA: Strange Effects on Invalid Rlist's. Empty Rlist: R15 loaded/stored, and Rb=Rb+40h. Writeback with Rb included in Rlist: Store OLD base if Rb is FIRST entry in Rlist, otherwise store NEW base,no writeback.
- Finish I/O viewer.
- Caution: A very large OBJ (of 128 pixels vertically, ie. a 64 pixels OBJ in a Double Size area) located at Y>128 will be treated as at Y>-128, the OBJ is then displayed parts offscreen at the TOP of the display, it is then NOT displayed at the bottom.
- Video Capture Mode (DMA3 only): Intended to copy a bitmap from memory (or from external hardware/camera) to VRAM. When using this transfer mode, set the repeat bit, and write the number of data units (per scanline) to the word count register. Capture works similar like HBlank DMA, however, the transfer is started when VCOUNT=2, it is then repeated each scanline, and it gets stopped when VCOUNT=162.
- Rumble.
- Fix simulated SWI 11h and 12h (offset). (?)

Random things
-------------

- Final Fantasy Tactics Advance. Mateus Totema LY effect is wrong.
- Reading a 32 bit value in ROM from EWRAM: 10 cycles (wait, wait, read instruction, compute address, wait, wait, wait, read low bits, wait, read high bits)
- Reading a 32 bit value in EWRAM from EWRAM: 10 cycles (wait, wait, read instruction, compute address, wait, wait, read low bits, wait, wait, read high bits)
- Loading a 16 bit value using mov/lsl/mov/orr: 12 cycles ( (wait, wait, read instruction) * 3 )



