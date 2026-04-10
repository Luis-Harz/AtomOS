# AtomOS
A OS written by me in C. <br>
It's really lightweight. <br>
## Why?
i made it because i wanted to learn a bit of Bare Metal C or also called freestanding C.
## Infos
It has about 3200 Lines of code right now. <br>
the filesystem is persistent. <br>
you have to emulate it with VGA(because it uses VGA Framebuffer, well now it kinda works on HDMI) <br>
It supports English and German keyboard layouts (note: ä, ö and ü don't work yet). <br>
It doesn't work on USB drives because USB driver is missing. <br>
SATA disks work. <br>
## TODO
[ ] USB Driver  
[ ] Desktop  
[X] ATA Driver  
[X] Multitasking

## Features

- VGA framebuffer graphics
- Persistent FAT filesystem
- ATA/SATA disk driver
- Keyboard and mouse support
- Basic multitasking
- Memory management
- PCI device enumeration
- RTC (real-time clock)
- Piezo buzzer support
- Built-in games: Snake, Pong

##FatFS
----------------------------------------------------------------------------
  FatFs - Generic FAT Filesystem Module  R0.16                               
----------------------------------------------------------------------------

Copyright (C) 2025, ChaN, all right reserved.

FatFs module is an open source software. Redistribution and use of FatFs in
source and binary forms, with or without modification, are permitted provided
that the following condition is met:

1. Redistributions of source code must retain the above copyright notice,
   this condition and the following disclaimer.
This software is provided by the copyright holder and contributors "AS IS"
and any warranties related to this software are DISCLAIMED.
The copyright owner or contributors be NOT LIABLE for any damages caused
by use of this software.
----------------------------------------------------------------------------
