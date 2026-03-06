# Terra Basic Kernel
### Author(s): Tristan Rush

## Reaching Long Mode
This topic relates more to bootloaders, but is important to know about when developing a kernel. When booting, computers start out in 16-bit Real mode, which provides direct access to the bios. In this mode, we can also place the graphics in VGA text mode. This lets us print text to our screen directly, without having to use a bitmapped font. In order to reach 32-bit protected mode, we need to ensure a few things first. One of the most unique settings that we need to set is the A20 Line.
The a20 line configures the first 20 bits of memory to behave in a fashion similar to the Intel 8086 architecture, where memory would wrap around to 0 if it was greater than 1MB.
Only AMD, Intel, and VIA processors can enter Long mode. This 64-bit mode can only be accessed once we set up 64-bit paging and disable 32-bit paging.

## Higher half 64-bit kernels
Placing the kernel within the higher half allows for cleaner memory management than if there kernel were dynamically assigned memory.
A higher half kernel also allows us to manage space allocated to a process much easier. 