# Terra Basic Graphics Driver
### Author(s): Tristan Rush

## Plotting pixels with Limine
We get the pointer to the first index of the framebuffer through the address field of the limine_framebuffer struct.
We can treat this pointer at a 1d array, using a standard formula that lets us find x and y coordinates from a 1d array to get the memory address in which we can then set our 32 bit color.

Limine puts us in graphics mode, so we must load a bitmap font either from a font file like .sfn or .psf or directly from a c file mapping the char's to the bytes.
