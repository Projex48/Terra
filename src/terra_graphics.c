#ifndef TERRA_GRAPHICS
#define TERRA_GRAPHICS

/*************************************/
// EXOs Graphics Driver

/***********************************************************/
#include <stddef.h>
#include <stdint.h>

/***********************************************************/
// Get framebuffer classes from Bootloader

#include "limine.h"

/***********************************************************/
// Local includes

#include "terra_syscalls.c"

/***********************************************************/
// Enums

enum color_palette {
  BLACK = 0x000000,
  GRAY = 0x212121,
  WHITE = 0xFFFFFF,
  BLUE = 0x0000FF,
  GREEN = 0x00FF00,
  RED = 0xFF0000,
};

/***********************************************************/
// Globals

// Set the base revision to 5, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(5);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

struct limine_framebuffer *framebuffer;

/***********************************************************/
// typedefs

typedef uint32_t size_type;

/***********************************************************/
// Simple graphics functions

// Initialize terminal
void graphics_initialize(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hcf();
  }

  // Ensure we got a framebuffer.
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  // Fetch the first framebuffer.
  framebuffer = framebuffer_request.response->framebuffers[0];
}

void setBackground(size_type VGA_COLOR) {
  volatile size_type *fb_ptr = framebuffer->address;
  size_type size = framebuffer->height * framebuffer->width;
  for (size_type i = 0; i < size; ++i) {
    fb_ptr[i] = VGA_COLOR;
  }
}

void createRectangle(size_type start_x, size_type start_y, size_type end_x,
                     size_type end_y, size_type VGA_COLOR) {
  volatile size_type *fb_ptr = framebuffer->address;
  for (size_type y = start_y; y < end_y; ++y) {
    for (size_type x = start_x; x < end_x; ++x) {
      fb_ptr[y * (framebuffer->pitch / 4) + x] = VGA_COLOR;
    }
  }
}

void putpixel(size_type pos_x, size_type pos_y, size_type VGA_COLOR) {
  volatile size_type *fb_ptr = framebuffer->address;
  fb_ptr[pos_y * (framebuffer->pitch / 4) + pos_x] = VGA_COLOR;
}

#endif